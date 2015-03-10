#ifndef COMMS_H
#define COMMS_H

#include "Arduino.h"

#include "data.h"
#include <XBee.h>

#define XBEE_SERIAL Serial1
#define XBEE_BAUD 9600
#define COORDINATOR_SH 0x0
#define COORDINATOR_SL 0x0

// Create a global reference to the xbee interface and other variables
static XBee xbee = XBee();
static XBeeAddress64 addr64 = XBeeAddress64(COORDINATOR_SH, COORDINATOR_SL);
static ZBTxStatusResponse txStatus = ZBTxStatusResponse();
static ZBTxRequest zbTx = ZBTxRequest(addr64, (uint8_t *)&payload, sizeof(payload));

// Initialize the xbee - to be called in setup()
void xbeeInit(){
    XBEE_SERIAL.begin(XBEE_BAUD);
    xbee.begin(XBEE_SERIAL);
}

// Send data using the XBee - should return 0 if success
static int xbeeSend(){
    xbee.getNextFrameId();
    xbee.send(zbTx);
    if(xbee.readPacket(5000)){
        // We got a response from our local xbee
        if(xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE){
            // This is the "all OK" route...
            xbee.getResponse().getZBTxStatusResponse(txStatus);
#if defined(DEBUG)
            if(txStatus.getDeliveryStatus() == SUCCESS){
                // All is well - output a message???
                Serial.println("Message sent over XBee successfully");
            } else {
                Serial.print("Unsuccessful attempt to send message over XBee. Error: ");
                Serial.println(txStatus.getDeliveryStatus());
            }
#endif
            return txStatus.getDeliveryStatus();
        } else {
#if defined(DEBUG)
            if(xbee.getResponse().isError()){
                Serial.print("Error reading packet from XBee. Error code: ");
                Serial.println(xbee.getResponse().getErrorCode());
            } else {
                // Unexpected response from the XBee - should we reset??
                Serial.println("Unexpected response from XBee after attempting to send data");
                Serial.println("Got a message with API ID of: ");
                Serial.println(xbee.getResponse().getApiID());
            }
#endif
        }
    }
    // Error...
    return -1;
}

#endif
