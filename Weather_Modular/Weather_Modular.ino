// Include libraries
#include <MPL3115A2.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <dht.h>

#include <XBee.h>

// Include the local files
#include "data.h"
#include "comms.h"
#include "sensors.h"

long nextReading = millis();
long interval = 5000;

void setup(){
  sensorsInit();
  xbeeInit();
  // lcdInit(); // to come
}

void loop() {
  if(millis() > nextReading){
    nextReading += interval;
    sensorsRead();
    fillPayload();
    if(xbeeSend() != 0) {
      Serial.println("Error sending data over XBee");
    } else {
      Serial.println("Send success");
    }
  } else {
    delay(100);
  }
}
