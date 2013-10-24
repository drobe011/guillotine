#ifndef GULLIOTINE_H_
#define GULLIOTINE_H_

#include <avr/io.h>
#include <avr/iom1284p.h>
#include <avr/interrupt.h>
#include <debug/debugserial.h>
#include "musicserial.h"
#include "blade.h"
#include "head.h"
#include "button.h"
#include "strobe.h"
#include "body.h"

#define LED_DDR DDRC
#define LED_PORT PORTC
#define LED_PIN 6

#define SETTIMERSEC(a) a*2
#define SETTIMERMIN(a) (a*2)*60


extern volatile uint16_t localTimer; //VARIABLE TO HOLD CURRENT TIMER TICK (10mS)
extern volatile uint16_t *ptr_Timer;  //POINTER TO ABOVE, ABLE TO PASS TO CLASS INSTANCES

__inline__ void setTimer(uint16_t tm = 0)
{
    cli();
    *ptr_Timer = tm;
    sei();
}
__inline__ uint16_t getTimer()
{
    uint16_t tmpTime = 0;
    cli();
    tmpTime = *ptr_Timer;
    sei();
    return tmpTime;
}
__inline__ void CONFIG_LED()
{
    LED_DDR |= _BV(LED_PIN);
}
__inline__ void LED_ON()
{
    LED_PORT |= _BV(LED_PIN);
}
__inline__ void LED_OFF()
{
    LED_PORT &= ~_BV(LED_PIN);
}


#endif // GULLIOTINE_H_
