#ifndef STROBE_H
#define STROBE_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define STROBE_PORT PORTB
#define STROBE_DDR DDRB
#define STROBE_PIN 1

#define STROBE_INIT_LENGTH 500

class Strobe
{
    public:
        Strobe(volatile uint16_t * ptr_T);
        void init();
        void strobeOn();
        void strobeOff();
    protected:
    private:
        volatile uint16_t * ptr_Timer;
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
        __inline__ void CONFIG_STROBE()
        {
            STROBE_DDR |= _BV(STROBE_PIN);
        }
        __inline__ void STROBE_ON()
        {
            STROBE_PORT |= _BV(STROBE_PIN);
        }
        __inline__ void STROBE_OFF()
        {
            STROBE_PORT &= ~_BV(STROBE_PIN);
        }
};

#endif // STROBE_H
