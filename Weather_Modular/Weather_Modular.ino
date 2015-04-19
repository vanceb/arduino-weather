// Include libraries


//#include <MPL3115A2.h> // Using a different Altitude sensor
#include <MS5611.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <dht.h>

#include <XBee.h>

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

// Include the local files
#include "data.h"
#include "comms.h"
#include "sensors.h"
#include "pwrsave.h"

long nextReading = millis();
long interval = 5000;

void setup(){
  pinMode(AWAKEPIN, OUTPUT);
  digitalWrite(AWAKEPIN, HIGH);
  delay(500);
  sensorsInit();
  xbeeInit();
  // lcdInit(); // to come
  Serial.begin(9600);
}

void loop() {
    sensorsRead();
    fillPayload();
    if(xbeeSend() != 0) {
      Serial.println("Error sending data over XBee");
    } else {
      Serial.println("Send success");
    }
    gotoSleep();
    // Give things time to come up after sleeping
    delay(50);
}
