#ifndef PWRSAVE_H
#define PWRSAVE_H

#include "Arduino.h"
#include "avr/sleep.h"
#include "avr/wdt.h"
#include "avr/power.h"

#include "comms.h"


// Have an LED to show we are awake
#define AWAKEPIN 13 // Most Arduinos have an LED on pin 13, so lets use it...

// Interrupt vector that handles WDT wake (If not handled causes a reset of the Arduino)
ISR(WDT_vect) {
  wdt_disable();
 
}

void gotoSleep() {
    // Turn off the indicator LED
    digitalWrite(AWAKEPIN, LOW);
    
    // Tell the XBee to sleep
    sleepXBee();
    
    // disable ADC
    ADCSRA &= ~(bit(ADEN)); // ADC
//    ADCSRB &= ~(bit(ADME)); // Analog Comparator
  
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
    
    // Turn on the indicator LED
    digitalWrite(AWAKEPIN, HIGH);
    
    // wake the XBee
    wakeXBee();
    
    // Re-enable the ADC
    ADCSRA |= bit(ADEN);
//    ADCSRB |= bit(ADME);
}

#endif
