#ifndef PWRSAVE_H
#define PWRSAVE_H

#include "Arduino.h"
#include "avr/sleep.h"
#include "avr/wdt.h"
#include "avr/power.h"

#define XBEE_SLEEP 7 // Setting Pin 7 low sends the XBee to PinSleep mode (Pin 9 on the XBee)
#define XBEE_SLEEP_CYCLES 6 // How many times we wake before the XBee is woken
int xbeeSleepCount = 0;

// Interrupt vector that handles WDT wake (If not handled causes a reset of the Arduino)
ISR(WDT_vect) {
  wdt_disable();
 
}

void sleepXBee(){
    // Signal to turn off the XBee
    pinMode(OUTPUT, XBEE_SLEEP);
    digitalWrite(XBEE_SLEEP, HIGH);    
}

// Returns the number of calls to this function before the XBee will actually wake - 0 means we woke it
int wakeXBee(bool force=false){
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
}

void gotoSleep() {
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
    
    // Don't wake XBee Automatically as we may do several cycles without sending data
    // User should call wakeXBee()
    
    // Re-enable the ADC
    ADCSRA |= bit(ADEN);
//    ADCSRB |= bit(ADME);
}

#endif
