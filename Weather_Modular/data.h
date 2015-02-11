#ifndef DATA_H
#define DATA_H
// Helper functions to convert floats to integers for easy transmission
#define FloatToInt16(f) (round((double)(f)*(double)100.0))

// Global variables to hold the data
// These will be updated in the main sketch
float inT1 = 0.0;
float inT2 = 0.0;
float outT = 0.0;
float pressure = 0.0;
uint8_t light = 0;

// Type to be used for transmission over xbee
typedef struct tagWeatherSample {
    // Time since we started
    unsigned long millis;

    // Pressure
    int32_t pressure;

    // Internal temps
    int16_t inT1;
    int16_t inT2;

    // External temp
    int16_t outT;

    // Ambient light
    uint16_t light;
} WeatherSample;

typedef struct tagXbeeCommBuffer {
    // Allow for future expansion of more than one message type
    #define WS_MSG_READING 'R'
    unsigned char msgType;

    // Data length
    byte length;

    // The data itself
    WeatherSample data;

} Payload;

// Create a global instance of the payload which will be updated
// by the fillPayload() function below
Payload payload;

void fillPayload (){
    payload.msgType = WS_MSG_READING;
    payload.length = sizeof(WeatherSample);
    payload.data.millis = millis();
    payload.data.inT1 = FloatToInt16(inT1);
    payload.data.inT2 = FloatToInt16(inT2);
    payload.data.outT = FloatToInt16(outT);
    payload.data.pressure = (int32_t)pressure;
    payload.data.light = light;
}

#endif
