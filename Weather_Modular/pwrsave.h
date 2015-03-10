#ifndef PWRSAVE_H
#define PWRSAVE_H

#include "Arduino.h"
#include "avr/sleep.h"
#include "avr/wdt.h"
#include "avr/power.h"

#define XBEE_SLEEP 7 // Setting Pin 7 low sends the XBee to PinSleep mode (Pin 9 on the XBee)

// Interrupt vector that handles WDT wake (If not handled causes a reset of the Arduino)
ISR(WDT_vect) {
  wdt_disable();
 
}

void wakeXBee(){
    // Turn on the XBee
    digitalWrite(XBEE_SLEEP, LOW); 
}

void gotoSleep() {
    // disable ADC
    ADCSRA = 0;
    
    // Send the XBee to sleep
    pinMode(OUTPUT, XBEE_SLEEP);
    digitalWrite(XBEE_SLEEP, HIGH);
  
    // clear various "reset" flags
    MCUSR = 0;
    // allow changes, disable reset
    WDTCSR = bit (WDCE) | bit (WDE);
    // set interrupt mode and an interval
    WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);
    // set WDIE, and 8 seconds delay
    wdt_reset(); // pat the dog

    set_sleep_mode (SLEEP_MODE_PWR_DOWN);
    noInterrupts ();
    // timed sequence follows
    sleep_enable();
    // turn off brown-out enable in software
    /*
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS);
    */
    interrupts (); // guarantees next instruction executed
    sleep_cpu ();

    // Now asleep until WDT wakes us...

    // cancel sleep as a precaution
    sleep_disable();
    
    // Don't wake XBee Automatically as we may do several cycles without sending data
    // User should call wakeXBee()
    
    // Re-enable the ADC
    ADCSRA = (ADCSRA | bit(ADEN));
}

#endif
