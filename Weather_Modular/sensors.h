#ifndef SENSORS_H
#define SENSORS_H

#include "Arduino.h"

// data.h defines the global variables we will fill
#include "data.h"

// Include pwrsave so we can use AWAKEPIN
#include "pwrsave.h"

// For the Dallas DS18B20 one wire Temp Sensors
#include <OneWire.h>
#include <DallasTemperature.h>

//For the MPL3115A2 I2C Temp and Pressure Sensor
#include <Wire.h>
//#include <MPL3115A2.h>
#include <MS5611.h>

// RHT-03 (DHT22)
#include <dht.h>

/*
 * Define the hardware pins used
 */
// LDR potential divider (Analog)
#define LDR A3

// Power measurement pins
#define BATTERY A1
#define SOLAR A0

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 10

// The MPL3115A2 uses the I2C bus which has pre-allocated pins

// Define Global variables to be used throughout the sketch
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature dallasTemp(&oneWire);
//
//MPL3115A2 mplTempPressure;
MS5611 tempPressure;

// RHT-03 uses a proprietory one wire protocol
// hooked up to pin 11
# define RHT03 11
dht rht03;

void sensorsInit(){
  // Start up the Dallas library
  dallasTemp.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

  // Start the MPL library
  //mplTempPressure.begin();
  
  //Configure it to measure atmospheric pressure
  //mplTempPressure.setModeBarometer();
  //mplTempPressure.setOversampleRate(7); // Set oversample to recommended 128
  //mplTempPressure.enableEventFlags();  
  
  
  // Start the MS5611
//  while (!tempPressure.begin()){
//    delay(500);
//    digitalWrite(AWAKEPIN, !digitalRead(AWAKEPIN));
//  }
  
  // no setup for the DHT required
}

void sensorsRead(){
  // Get the power related readings
  solar = analogRead(SOLAR);
  battery = analogRead(BATTERY);
  
  // Get the ambient light reading
  light = analogRead(LDR);
  
  // Read the temperatures from the Dallas sensors
  dallasTemp.requestTemperatures(); // Send the command to get temperatures
  inT1 = dallasTemp.getTempCByIndex(0);
  outT = dallasTemp.getTempCByIndex(1);
  
  // Read the temperature and pressure from the MLP3115A2
  //inT2 = mplTempPressure.readTemp();
  //pressure = mplTempPressure.readPressure(); // Could Divide by 100 to get mBar
  
  // Read Temerature and Pressure form the MS5611
  //inT2 = tempPressure.readTemperature();
  //pressure = tempPressure.readPressure()/1.0;
  
  // Read temp and humidity from the DHT
  int chk = rht03.read22(RHT03);
  inT3 = rht03.temperature;
  humidity = rht03.humidity;
}

#endif
