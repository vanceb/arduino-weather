#ifndef COMMS_H
#define COMMS_H

#include "Arduino.h"

#include "data.h"
#include <XBee.h>

#define XBEE_SERIAL Serial
#define XBEE_BAUD 9600
#define COORDINATOR_SH 0x0
#define COORDINATOR_SL 0x0

#define XBEE_SLEEP 7 // Setting Pin 7 high sends the XBee to PinSleep mode (Pin 9 on the XBee)
#define CTS 6 // CTS indicates that the XBee is good to receive data
#define TX_TIMEOUT 1000

#define XBEE_SLEEP_CYCLES 0 // Not fully implemented yet, so set to 0 for wake every time
int xbeeSleepCount = 0;

// Create a global reference to the xbee interface and other variables
static XBee xbee = XBee();
static XBeeAddress64 addr64 = XBeeAddress64(COORDINATOR_SH, COORDINATOR_SL);
static ZBTxStatusResponse txStatus = ZBTxStatusResponse();
static ZBTxRequest zbTx = ZBTxRequest(addr64, (uint8_t *)&payload, sizeof(payload));

// Initialize the xbee - to be called in setup()
void xbeeInit(){
    // Make sure the XBee is awake
    pinMode(XBEE_SLEEP, OUTPUT);
    digitalWrite(XBEE_SLEEP, LOW);
    // Setup CTS Pin
    pinMode(CTS, INPUT);
    
   // Initialize the comms   
    XBEE_SERIAL.begin(XBEE_BAUD);
    xbee.begin(XBEE_SERIAL);
}

void sleepXBee(){
    // Signal to turn off the XBee
    pinMode(XBEE_SLEEP, OUTPUT);
    digitalWrite(XBEE_SLEEP, HIGH);    
}

// Returns the number of calls to this function before the XBee will actually wake - 0 means we woke it
int wakeXBee(bool force=false){
  /*
    // We are not going to wake the XBee every time... (Unless forced)  
    if (force == true) {
      xbeeSleepCount = 0;
    } else {
      if (xbeeSleepCount <= 0) {
        // We woke last time and need to reset
        xbeeSleepCount = XBEE_SLEEP_CYCLES;
      } else {
        xbeeSleepCount -= 1;
      }
    }
    
    if (xbeeSleepCount <= 0) {
      // Turn on the XBee
      digitalWrite(XBEE_SLEEP, LOW);  
    }
    
    return xbeeSleepCount;
    */
    pinMode (XBEE_SLEEP, OUTPUT);
    digitalWrite(XBEE_SLEEP, LOW);
}

// Send data using the XBee - should return 0 if success
static int xbeeSend(){
   // Make sure the XBee is awake
    wakeXBee();
    // Wait for CTS from the XBee or timeout
    bool gotCTS = false;
    long timeout = millis() + TX_TIMEOUT;
    while(millis() < timeout) {
      if (digitalRead(CTS) == false) {
        gotCTS = true;
        break;
      } else {
        delay(10);
      }
    }

    if(gotCTS == true){
      xbee.getNextFrameId();
      xbee.send(zbTx);
      if(xbee.readPacket(5000)){
          // We got a response from our local xbee
          if(xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE){
              // This is the "all OK" route...
              xbee.getResponse().getZBTxStatusResponse(txStatus);
              return txStatus.getDeliveryStatus();
          } 
      }
    }
    // Error...
    return -1;
}

#endif
