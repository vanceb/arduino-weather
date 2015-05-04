// Include libraries


#include <MPL3115A2.h> // Using a different Altitude sensor
//#include <MS5611.h>
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
  // Turn on the LED
  pinMode(AWAKEPIN, OUTPUT);
  digitalWrite(AWAKEPIN, HIGH);
  // Make sure the XBee is awake and ready
  xbeeInit();
  // Give things time to settle
  delay(500);
  sensorsInit();
  // lcdInit(); // to come
}

void loop() {
    sensorsRead();
    fillPayload();
    xbeeSend();
    //Give the xbee time to respond and send the message before sending to to sleep
    delay(100);
    gotoSleep();
    // Give things time to come up after sleeping
    delay(50);
}
