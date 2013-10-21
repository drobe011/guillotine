#ifndef HEAD_H
#define HEAD_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <debug/debugserial.h>

#define HEAD_UP_MAX_TIME 1500
#define HEAD_DOWN_MAX_TIME 200
#define HOIST_DOWN_MAX_TIME 1500
#define TILT_UP_MAX_TIME 600
#define TILT_DOWN_MAX_TIME 600
#define RELEASE_ON_MAX_TIME 50

#define TILT_SPEED_UP 127
#define TILT_SPEED_DN 50
#define TILT_MTR_BRAKE_HOLD 50

#define HOIST_PORT PORTD
#define HOIST_DDR DDRD
#define HOIST_PIN 5
#define DIR_PORT PORTC
#define DIR_DDR DDRC
#define DIR_PIN 0
#define RELEASE_PORT PORTD
#define RELEASE_DDR DDRD
#define RELEASE_PIN 4
#define HOIST_ENABLE_PORT PORTD
#define HOIST_ENABLE_DDR DDRD
#define HOIST_ENABLE_PIN 7

//----------------------
#define TILT_PORT PORTB
#define TILT_DDR DDRB
#define TILT_PIN 3
#define TILT_IN2_PORT PORTA
#define TILT_IN2_DDR DDRA
#define TILT_IN2_PIN 1
#define TILT_IN1_PORT PORTA
#define TILT_IN1_DDR DDRA
#define TILT_IN1_PIN 0
#define TILT_EN_PORT PORTB
#define TILT_EN_DDR DDRB
#define TILT_EN_PIN 4
//------------------------

#define IS_HOIST_DOWN_PORT PIND
#define IS_HOIST_DOWN_PU PORTD
#define IS_HOIST_DOWN_PIN 6
#define IS_HEAD_DOWN_PORT PINA
#define IS_HEAD_DOWN_PIN 5
#define IS_HEAD_UP_PORT PINA
#define IS_HEAD_UP_PIN 4
#define IS_HOIST_FAULT_PORT PINB
#define IS_HOIST_FAULT_PIN 2

//----------------------------
#define IS_TILT_UP_PORT PINB
#define IS_TILT_UP_PU PORTB
#define IS_TILT_UP_PIN 7
#define IS_TILT_DOWN_PORT PINB
#define IS_TILT_DOWN_PU PORTB
#define IS_TILT_DOWN_PIN 5
//----------------------------

class Head
{
    public:
        Head(volatile uint16_t * ptr_T);
        uint8_t init();
        uint8_t raiseHeadPoll(uint8_t = 0);
        uint8_t lowerHeadPoll(uint8_t = 0);
        uint8_t lowerHoistPoll(uint8_t = 0);
        uint8_t tiltHeadUpPoll(uint8_t = 0);
        uint8_t tiltRodDownPoll(uint8_t = 0);
        void turnOff();
        void sol(uint8_t);
        void printDebug(DebugSerial *);
        enum {ERROR, DONE, COMPLETE, WORKING, NOT_STARTED, RESET};
    protected:
    private:
        volatile uint16_t * ptr_Timer;
        void configHoistTimer();
        void configTiltTimer();
        uint8_t status;
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
        __inline__ void CONFIG_HOIST()
        {
            HOIST_DDR |= _BV(HOIST_PIN);
        }
        __inline__ void CONFIG_RELEASE()
        {
            RELEASE_DDR |= _BV(RELEASE_PIN);
        }
        __inline__ void CONFIG_HOIST_ENABLE()
        {
            HOIST_ENABLE_DDR |= _BV(HOIST_ENABLE_PIN);
        }
        __inline__ void CONFIG_DIR()
        {
            DIR_DDR |= _BV(DIR_PIN);
        }
        __inline__ void CONFIG_TILT()
        {
            TILT_DDR |= _BV(TILT_PIN);
        }
        __inline__ void CONFIG_TILT_DIR()
        {
            TILT_IN1_DDR |= _BV(TILT_IN1_PIN);
            TILT_IN2_DDR |= _BV(TILT_IN2_PIN);
        }
        __inline__ void CONFIG_TILT_EN()
        {
            TILT_EN_DDR |= _BV(TILT_EN_PIN);
        }
        __inline__ void CONFIG_TILT_SNSRS()
        {
            IS_TILT_DOWN_PU |= _BV(IS_TILT_DOWN_PIN);
            IS_TILT_UP_PU |= _BV(IS_TILT_UP_PIN);
            IS_HOIST_DOWN_PU |= _BV(IS_HOIST_DOWN_PIN);
        }
        __inline__ void DIR_FORWARD()
        {
            DIR_PORT |= _BV(DIR_PIN);
        }
        __inline__ void DIR_REVERSE()
        {
            DIR_PORT &= ~_BV(DIR_PIN);
        }
        __inline__ void RELEASE_ON()
        {
            RELEASE_PORT |= _BV(RELEASE_PIN);
        }
        __inline__ void RELEASE_OFF()
        {
            RELEASE_PORT &= ~_BV(RELEASE_PIN);
        }
        __inline__ void HOIST_ON()
        {
            HOIST_ENABLE_PORT |= _BV(HOIST_ENABLE_PIN);
        }
        __inline__ void HOIST_OFF()
        {
            HOIST_ENABLE_PORT &= ~_BV(HOIST_ENABLE_PIN);
        }
        __inline__ void TILT_OFF()
        {
            TILT_EN_PORT &= ~_BV(TILT_EN_PIN);
        }
        __inline__ void TILT_ON()
        {
            TILT_EN_PORT |= _BV(TILT_EN_PIN);
        }
        __inline__ void TILT_DOWN()
        {
            TILT_IN2_PORT |= _BV(TILT_IN2_PIN);
            TILT_IN1_PORT &= ~_BV(TILT_IN1_PIN);
            OCR0A = TILT_SPEED_DN;
        }
        __inline__ void TILT_UP()
        {
            TILT_IN2_PORT &= ~_BV(TILT_IN2_PIN);
            TILT_IN1_PORT |= _BV(TILT_IN1_PIN);
            OCR0A = TILT_SPEED_UP;
        }
        __inline__ uint8_t IS_HOIST_DOWN()
        {
            return !(IS_HOIST_DOWN_PORT & _BV(IS_HOIST_DOWN_PIN));
        }
        __inline__ void TILT_BRAKE()
        {
            TILT_IN2_PORT |= _BV(TILT_IN2_PIN);
            TILT_IN1_PORT |= _BV(TILT_IN1_PIN);
            OCR0A = 255;
        }
        __inline__ uint8_t IS_HEAD_UP()
        {
            return !(IS_HEAD_UP_PORT & _BV(IS_HEAD_UP_PIN));
        }
        __inline__ uint8_t IS_HEAD_DOWN()
        {
            return !(IS_HEAD_DOWN_PORT & _BV(IS_HEAD_DOWN_PIN));
        }
        __inline__ uint8_t IS_HOIST_FAULT()
        {
            return IS_HOIST_FAULT_PORT & _BV(IS_HOIST_FAULT_PIN);
        }
        __inline__ uint8_t IS_TILT_DOWN()
        {
            return !(IS_TILT_DOWN_PORT & _BV(IS_TILT_DOWN_PIN));
        }
        __inline__ uint8_t IS_TILT_UP()
        {
            return !(IS_TILT_UP_PORT & _BV(IS_TILT_UP_PIN));
        }

};

#endif // HEAD_H
