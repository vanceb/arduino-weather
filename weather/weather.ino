// For the Dallas DS18B20 one wire Temp Sensors
#include <OneWire.h> 
#include <DallasTemperature.h> 
//For the MPL3115A2 I2C Temp and Pressure Sensor
#include <Wire.h>
#include "MPL3115A2.h"
// For the LCD Display
#include <LiquidCrystal.h>

// Debug Flag
#define DEBUG 0
//#define TO_SERIAL 1
#define VERSION 0.1

#define BACKLIGHT 13
int light = A0;

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 10

// Define Global variables to be used throughout the sketch
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature dallasTemp(&oneWire);
// 
MPL3115A2 mplTempPressure;

// LCD
LiquidCrystal lcd(9,8,7,6,5,4);
// Create the "Degree" character
uint8_t degree[8] = {
  B00010,
  B00101,
  B00101,
  B00010,
  B00000,
  B00000,
  B00000,
  B00000,
};
// Create a down arrow
uint8_t falling[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100,
  B00000,
};
// Create an up arrow
uint8_t rising[8] = {
  B00000,
  B00100,
  B01110,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
};
// Create a symbol for stable
uint8_t stable[8] = {
  B00000,
  B00100,
  B01110,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000,
};
// Create a symbol for unknown
uint8_t empty[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
};
// Variables to hold the values
float inTemp = 0.0;
float inTemp2 = 0.0;
float outTemp = 0.0;
float pressure = 0.0;

// Hold historic values to help error checking
float inTempHist = 0.0;
float inTemp2Hist = 0.0;
float outTempHist = 0.0;
float pressureHist = 0.0;

boolean firstRun = true;
float maxVariation = 0.5; // difference between readings, should be good for both temp an pressure
int smooth = 5; // How much weighting the historic value gets vs last reading (Smoothing)

// Keep a historic record
int const readingsPerMin = 12; //number of readings per minute
float readings[readingsPerMin];
float mins[60];
float hours[24];
// Indexes to next value to be written
int rdgIndex = 0;
int minIndex = 0;
int hrIndex = 0;

// Long historic value to allow estimate of rising or falling pressure
float longPressureHist = 0.0;
float pressureChangeMin = 0.1;
long measureTime = 0;
long measureInterval = 60000/readingsPerMin; // 5 second intervals between measurements
long loopCount = 0;
//long loops = 1*60*60/(measureTime/1000); //about 1 hour
long loops = 12;



// This helper function allows for smoothing of displayed values
// and the updating of historic record
float updateT(float hist, float value){
  float newVal = 0.0;
  if(value < hist + maxVariation && value > hist - maxVariation){
    newVal = (smooth*hist + value)/(smooth+1.0);
  } else {
    newVal = hist;
  }
  return newVal;
}

float updateP(float hist, float value){
  float newVal = updateT(hist, value);
  float sum=0.0;
  rdgIndex++;
  if(rdgIndex == readingsPerMin){ // We have filled the readings buffer
    // Roll the pointer round
    rdgIndex = 0;
    minIndex++;
    if(minIndex == 60){ // We have filled the minutes buffer
      // Roll the pointer round
      minIndex = 0;
      hrIndex++; 
      if(hrIndex == 24){ // We have filled the hours buffer
        hrIndex = 0;
      }
      sum = 0.0;
      for (int i=0; i<60; i++){
        sum += mins[i];
      }
      hours[hrIndex] = sum/60.0;
    }
    sum = 0.0;
    for(int i=0; i< readingsPerMin; i++){
      sum += readings[i];
    }
    mins[minIndex] = sum/readingsPerMin;
  }  
  readings[rdgIndex] = newVal;
  return newVal;
}

float hrChange(int age){
  int histIndex = (hrIndex + (24 - age)) % 24;
  if(hours[hrIndex] < 0.0 || hours[histIndex] < 0.0){
    return -9999.0;
  } else {
    return hours[hrIndex] - hours[histIndex];
  }
}

float minChange(int age){
  int histIndex = (minIndex + (60 - age)) % 60;
  if(mins[minIndex] < 0.0 || mins[histIndex] < 0.0){
    return -9999.0;
  } else {
    return mins[minIndex] - mins[histIndex];
  }
}

byte symbol (float diff, float tolerance){
  if (diff < -9000){ // Comparison with uninitialised value
    return byte(4);
  } else if(diff > tolerance){
    return byte(1);
  } else if (diff < -tolerance) {
    return byte(2);
  } else {
    return byte(3);
  }
}

// a helper function for placing temperature readings on the LCD
int startTemp(float temp){
  int start = 0;
  // Adjust start position for Right Justify
  if(temp > 0){
    start++;
  }
  if(abs(temp) < 9.95){
    start++;
  }
  return start;
}

void printSerial(String label, float inT1, float inT2, float outT, float p){
  Serial.print(label);
  Serial.print(inT1);
  Serial.print(", ");
  Serial.print(inT2);
  Serial.print(", ");
  Serial.print(outT);
  Serial.print(", ");
  Serial.println(p);
}

void printLCD(float inT1, float inT2, float outT, float p){
  lcd.clear();
  lcd.setCursor(startTemp(inT1),0);
  lcd.print(inT1,1);
  lcd.write(byte(0)); // degree symbol
  lcd.print("C");
  lcd.setCursor(9 + startTemp(inT2),0);
  lcd.print(outT,1);
  lcd.write(byte(0)); // degree symbol
  lcd.print("C");
  if(p > 999.95){ // Account for rounding...
    lcd.setCursor(0,1);
  } else {
    lcd.setCursor(1,1);
  }
  lcd.print(p,1);
  lcd.print(" mb");
  #if DEBUG > 1
    lcd.setCursor(10,1);
    lcd.print(loopCount);
    lcd.setCursor(15,1);
    lcd.write(symbol(minChange(30), 0.2));
  #else
    lcd.setCursor(13,1);
    lcd.write(symbol(hrChange(23), 1.0));
    lcd.write(symbol(hrChange(6), 0.5));
    lcd.write(symbol(minChange(30), 0.2));
  #endif
}
  
  
void setup(void)
{
  #if defined(TO_SERIAL)
  // start serial port
  Serial.begin(9600);
  #endif

  // Start up the Dallas library
  dallasTemp.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

  // Start the MPL library
  mplTempPressure.begin();
  //Configure it to measure atmospheric pressure
  mplTempPressure.setModeBarometer();
  mplTempPressure.setOversampleRate(7); // Set oversample to recommended 128
  mplTempPressure.enableEventFlags();
  
  //LCD
  pinMode(BACKLIGHT, OUTPUT);
  analogWrite(BACKLIGHT, 128);
  pinMode(light, INPUT);
  lcd.begin(16, 2);
  lcd.print("Weather Station");
  lcd.setCursor(0,1);
  lcd.print("Version ");
  lcd.print(VERSION);
  lcd.createChar(0,degree);
  lcd.createChar(1,rising);
  lcd.createChar(2,falling);
  lcd.createChar(3,stable);
  lcd.createChar(4,empty);
  delay(1000);
  
  // Zero out the history arrays
  for(int i=0; i<readingsPerMin; i++)
    readings[i] = -3000.0;
  for(int i=0; i<60; i++)
    mins[i] = -3000.0;
  for(int i=0; i<24; i++)
    hours[i] = -3000.0;
}


void loop(void)
{ 
  long start = millis();

  // Set the backlight depending on the ambient light
  int bright = analogRead(light);
  analogWrite(BACKLIGHT, map(bright,0,1000,5,255));
  
  // Possibly write out debug information
   #if DEBUG > 3
     #if defined(TO_SERIAL)
      Serial.println(start);
      Serial.print("Bright: ");
      Serial.println(bright);
      delay(10);
    #endif
  #endif
  
  if (millis() > measureTime){
    //Ask the dallas sensors to perform a reading
    dallasTemp.requestTemperatures(); // Send the command to get temperatures
    inTemp = dallasTemp.getTempCByIndex(0);
    outTemp = dallasTemp.getTempCByIndex(1);
    
    // Ask the MPL to perform a reading
    inTemp2 = mplTempPressure.readTemp();
    pressure = mplTempPressure.readPressure()/100; // Divide by 100 to get mBar
    
    // Do some error checking - regularly see 0.0 coming back from the pressure sensor...
    if(firstRun){
      int check = 0;
      if(pressure > 500.0){
        pressureHist = pressure;
        longPressureHist = pressure;
        check++;
      }
      if(inTemp > -20.0){
        inTempHist = inTemp;
        check++;
      }
      if(inTemp2 > -20.0){
        inTemp2Hist = inTemp2;
        check++;
      }
      if(outTemp > -50.0){
        outTempHist = outTemp;
        check++;
      }
      if(check == 4){
        firstRun=false;
      }
    } else {
      pressureHist = updateP(pressureHist, pressure);
      inTempHist = updateT(inTempHist, inTemp);
      inTemp2Hist = updateT(inTemp2Hist, inTemp2);
      outTempHist = updateT(outTempHist, outTemp);
    }
    
    printLCD(inTempHist, inTemp2Hist, outTempHist, pressureHist);
    
    #if defined(TO_SERIAL)
      printSerial("raw  ", inTemp, inTemp2, outTemp, pressure);
      printSerial("hist ", inTempHist, inTemp2Hist, outTempHist, pressureHist);
    #endif
    
    loopCount++;

    // Debug information to Serial
    #if DEBUG > 3
      #if defined(TO_SERIAL)
        long processTime = millis() - start;
        Serial.print("Milliseconds for the loop: ");
        Serial.println(processTime);
        processTime++; //we have taken about 1ms here so compensate :-)
      #endif
    #endif
    
    while (measureTime < millis()){
      measureTime += measureInterval;
    }
  }
}
