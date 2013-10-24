#ifndef BODY_H
#define BODY_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <debug/debugserial.h>

#define BODY_PORT PORTB
#define BODY_DDR DDRB
#define BODY_PIN 6
#define BODY_ENABLE_PORT PORTD
#define BODY_ENABLE_DDR DDRD
#define BODY_ENABLE_PIN 7
#define BODY_DIR_PORT PORTC
#define BODY_DIR_DDR DDRC
#define BODY_DIR_PIN 1
#define BODY_FAULT_PORT PINB
#define BODY_FAULT_PIN 2

#define DEFAULT_DUTY 255
#define BODY_INIT_LENGTH 100
#define BODY_KICK_LENGTH 200

class Body
{
    public:
        Body(volatile uint16_t * ptr_T);
        uint8_t init();
        uint8_t bodyOn(uint8_t = DEFAULT_DUTY, uint8_t = 1);
        void turnOff();
        //uint8_t bodyReverse();
        uint8_t bodyKickPoll(uint8_t = 0);
        void bodyTwitch();
        void printDebug(DebugSerial *);
        enum {ERROR, DONE, COMPLETE, WORKING, NOT_STARTED, RESET};
    protected:
    private:
        volatile uint16_t * ptr_Timer;
        uint8_t bodyStatus;
        void configTimer();
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
        __inline__ void CONFIG_BODY()
        {
            BODY_DDR |= _BV(BODY_PIN);
        }
        __inline__ void CONFIG_ENABLE()
        {
            BODY_ENABLE_DDR |= _BV(BODY_ENABLE_PIN);
        }
        __inline__ void CONFIG_DIR()
        {
            BODY_DIR_DDR |= _BV(BODY_DIR_PIN);
        }
        __inline__ void DIR_FORWARD()
        {
            BODY_DIR_PORT |= _BV(BODY_DIR_PIN);
        }
        __inline__ void DIR_REVERSE()
        {
            BODY_DIR_PORT &= ~_BV(BODY_DIR_PIN);
        }
        __inline__ void BODY_ON()
        {
            BODY_ENABLE_PORT |= _BV(BODY_ENABLE_PIN);
        }
        __inline__ void BODY_OFF()
        {
            BODY_ENABLE_PORT &= ~_BV(BODY_ENABLE_PIN);
        }
        __inline__ uint8_t IS_BODY_FAULT()
        {
            return BODY_FAULT_PORT & _BV(BODY_FAULT_PIN);
        }
        __inline__ void REVERSE_DIR_IN_STEP()
        {
            BODY_DIR_PORT ^= _BV(BODY_DIR_PIN);
        }
};

#endif // BODY_H
