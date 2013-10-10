#ifndef BLADE_H
#define BLADE_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define BLADE_UP_PORT PORTB
#define BLADE_UP_DDR DDRB
#define BLADE_UP_PIN 0
#define MAG_DDR DDRA
#define MAG_PORT PORTA
#define MAG_PIN 2
#define IS_BLADE_DOWN_PORT PINA
#define IS_BLADE_DOWN_PIN 7
#define IS_BLADE_UP_PORT PINA
#define IS_BLADE_UP_PIN 6

#define BLADE_MTR_PAUSE_TOP 100
#define BLADE_UP_MAX_TIME 500
#define BLADE_DOWN_MAX_TIME 300

class Blade
{
    public:
        Blade(volatile uint16_t * ptr_T);
        uint8_t init();
        void raiseBlade();
        void lowerBlade();
        void motorOff();
        uint8_t checkIfUp();
        uint8_t checkIfDown();

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
        __inline__ void CONFIG_BLADE_UP()
        {
            BLADE_UP_DDR |= _BV(BLADE_UP_PIN);
        }
        __inline__ void CONFIG_MAG()
        {
            MAG_DDR |= _BV(MAG_PIN);
        }
        __inline__ void BLADE_MOTOR_ON()
        {
            BLADE_UP_PORT |= _BV(BLADE_UP_PIN);
        }
        __inline__ void BLADE_MOTOR_OFF()
        {
            BLADE_UP_PORT &= ~_BV(BLADE_UP_PIN);
        }
        __inline__ void BLADE_HOLD()
        {
            MAG_PORT |= _BV(MAG_PIN);
        }
        __inline__ void BLADE_RELEASE()
        {
            MAG_PORT &= ~_BV(MAG_PIN);
        }
        __inline__ uint8_t IS_BLADE_DOWN()
        {
            return !(IS_BLADE_DOWN_PORT & _BV(IS_BLADE_DOWN_PIN));
        }
        __inline__ uint8_t IS_BLADE_UP()
        {
            return !(IS_BLADE_UP_PORT & _BV(IS_BLADE_UP_PIN));
        }
};

#endif // BLADE_H

