#ifndef DATA_H
#define DATA_H

#include "Arduino.h"

// Helper functions to convert floats to integers for easy transmission
#define FloatToInt16(f) (round((double)(f)*(double)100.0))

// Define constants used in the messages to help the receiving app
// Allow for future expansion of more than one message type
#define APP_ID 0x10A1 // Weather Sensing application
#define WS_MSG_READING 0x0001 // a Reading


// Global variables to hold the sensor data
// These will be updated in the main sketch
float inT1 = 0.0;
float inT2 = 0.0;
float inT3 = 0.0;
float outT = 0.0;
float pressure = 0.0;
float humidity = 0.0;
uint16_t light = 0;
uint16_t battery = 0;
uint16_t solar = 0;

// Type to be used for transmission over xbee
typedef struct tagWeatherSample {
    // Time since we started
    unsigned long millis;

    // Pressure
    int32_t pressure;

    // Internal temps
    int16_t inT1;
    int16_t inT2;
    int16_t inT3;

    // External temp
    int16_t outT;
    
    // Humidity
    uint16_t humidity;

    // Ambient light
    uint16_t light;
    
    // Power
    uint16_t battery;
    uint16_t solar;

} WeatherSample;

typedef struct tagXbeeCommBuffer {

    // Header data
    uint16_t appID;
    uint16_t msgType;
    uint16_t reserved; // included as it maked decoding easier by placing the header on a 32 bit boundary
    
    // Data length
    uint16_t length; //could be just a byte, but keeping it on a 16 bit boundary helps decoding

    // The data itself
    WeatherSample data;

} Payload;

// Create a global instance of the payload which will be updated
// by the fillPayload() function below
Payload payload;

void fillPayload (){
    payload.appID = APP_ID;
    payload.msgType = WS_MSG_READING;
    payload.reserved = 0x0;
    payload.length = sizeof(WeatherSample);
    payload.data.millis = millis();
    payload.data.inT1 = FloatToInt16(inT1);
    payload.data.inT2 = FloatToInt16(inT2);
    payload.data.inT3 = FloatToInt16(inT3);
    payload.data.outT = FloatToInt16(outT);
    payload.data.pressure = (int32_t)pressure;
    payload.data.humidity = FloatToInt16(humidity);
    payload.data.light = light;
    payload.data.battery = battery;
    payload.data.solar = solar;
}

#endif
