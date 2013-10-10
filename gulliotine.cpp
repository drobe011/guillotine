
#define DEBUG

#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef DEBUG
#include <debug/debugserial.h>
#endif // DEBUG
#include "blade.h"
#include "head.h"
#include "button.h"

#define LED_DDR DDRC
#define LED_PORT PORTC
#define LED_PIN 6

//SET UP GLOBAL VARIABLES
volatile uint16_t localTimer; //VARIABLE TO HOLD CURRENT TIMER TICK
volatile uint16_t * ptr_Timer = &localTimer;  //POINTER TO ABOVE, ABLE TO PASS TO CLASS INSTANCES

inline void setTimer(uint16_t tm = 0)
{
    cli();
    *ptr_Timer = tm;
    sei();
}
inline uint16_t getTimer()
{
    uint16_t tmpTime = 0;
    cli();
    tmpTime = *ptr_Timer;
    sei();
    return tmpTime;
}
inline void CONFIG_LED()
{
    LED_DDR |= _BV(LED_PIN);
}
inline void LED_ON()
{
    LED_PORT |= _BV(LED_PIN);
}
inline void LED_OFF()
{
    LED_PORT &= ~_BV(LED_PIN);
}

int main(void)
{
    //SET UP PIN MODES
    #ifdef DEBUG
    DebugSerial DBSerial = DebugSerial();
    #endif // DEBUG

    //SET UP GP TIMER
    TCCR2A = 0b00000010; //CTC MODE
    TCCR2B = 0b00000111; // /1024
    TIMSK2 = 0b00000010; // CTCIE
    OCR2A = 155;  //100.16026 Hz (~10mS)
    //SET UP STATUS LED
    CONFIG_LED();

    /////////////////////////////////////////
    //SET UP
    enum Vargs {INIT, WAIT, CHECK, RAISE, LOWER, CHECKRAISE, CHECKLOWER, HEADRAISE, HEADLOWER, CHECKHDRAISE, CHECKHDLOWER, \
        HEADTILT, CHECKHDTILT, LOWERTILTROD, CHECKLOWERTILTROD, ERROR};

    //*_LONG = SECONDS
    //*_SHORT = 10mS
    const uint8_t CHOP_PAUSE_LONG = 12; //pause between reset all and blade lower
    const uint8_t RAISE_PAUSE_LONG = 30; //pause after blade lower to raise again
    const uint8_t HEAD_RAISE_PAUSE_LONG = 30; //pause after blade raised to raise head
    const uint8_t HEAD_TILT_PAUSE_LONG = 30; //pause after head raise to tilt head into lock

    Blade myBlade = Blade(ptr_Timer);
    Head myHead = Head(ptr_Timer);
    Button myButton = Button(ptr_Timer);
    Vargs state = INIT;
    Vargs nextState = WAIT;
    uint16_t longTime = 0;
    uint8_t forceNextState = 0;
    uint8_t isTest = 0;

    sei();

    if (myButton.read()) isTest = 1;

    while(1)
    {
        /*START BASIC STATE MACHINE
            state = actual state
            nextState = next state after time delay is up
          STATE MAP
            INIT
          * WAIT
                LOWER
            LOWER
            CHECKLOWER
            HEADLOWER
            CHECKHDLOWER
            BODYTWITCH**
            WAIT
                ENDBODYTWITCH**
            ENDBODYTWITCH**
            WAIT
                RAISE
            RAISE
            CHECKRAISE
            WAIT
                HEADRAISE
            HEADRAISE
            CHECKHDRAISE
            WAIT
                HEADTILT
            HEADTILT
            CHECKHDTILT
            WAIT
                LOWERTILTROD
            LOWERTILTROD
            CHECKLOWERTILTROD
            WAIT
                LOWERHOIST**
            LOWERHOIST**
            CHECKLOWERHOIST**
            BACK TO * WAIT
        */
        switch (state)
        {
        //INIT: SET GUILLOTINE TO DEFAULT POSITIONS
        //      BLADE UP, HEAD UP, HEAD TILT UP
        case INIT:
            LED_OFF();
            if (!myBlade.init())        //MOVE BLADE TO UP POSITION
            {
                state = ERROR;
                break;
            }
            else if (!myHead.init(0))   //MOVE HEAD TILT ROD TO HOME POSITION
            {
                state = ERROR;
                break;
            }
            else if (!myHead.init(1))   //MOVE HEAD AND HEAD TILT TO UP POSITION
            {
                state = ERROR;
                break;
            }
            else if (!myHead.init(0))   //MOVE HEAD TILT ROD TO HOME POSITION
            {
                state = ERROR;
                break;
            }
            state = WAIT;
            nextState = LOWER;
            setTimer();
            break;

        //WAIT: WAIT UNTIL TIME TO DO nextState
        //      IF BUTTON PRESS, MOVE TO NEXT STATE NOW
        case WAIT:
            if (getTimer() >= 100)
            {
                longTime++;
                setTimer();

            }
            if (myButton.read()) forceNextState = 1;
            switch (nextState)
            {
            case LOWER:
                if (longTime >= CHOP_PAUSE_LONG || forceNextState)
                {
                    state = LOWER;
                    longTime = 0;
                    forceNextState = 0;
                }
                break;

            case RAISE:
                if (longTime >= RAISE_PAUSE_LONG  || forceNextState)
                {
                    state = RAISE;
                    longTime = 0;
                    forceNextState = 0;
                }
                break;
            case HEADRAISE:
                if (longTime >= HEAD_RAISE_PAUSE_LONG || forceNextState)
                {
                    state = HEADRAISE;
                    longTime = 0;
                    forceNextState = 0;
                }
                break;
            }
            break;

        case CHECK:

            break;

        case RAISE:
            if (myBlade.checkIfDown())
            {
                myBlade.raiseBlade();
                setTimer();
                state = CHECKRAISE;

            }
            else state = ERROR;
            break;

        case LOWER:
            if (myBlade.checkIfUp())
            {
                myBlade.lowerBlade();
                state = CHECKLOWER;

            }
            else state = ERROR;
            break;

        case CHECKRAISE:
            if (getTimer() >= BLADE_UP_MAX_TIME)
            {
                state = ERROR;
                break;
            }
            if (myBlade.checkIfUp())
            {
                setTimer();
                while (getTimer() < BLADE_MTR_PAUSE_TOP)
                {
                    if (myButton.read()) break; //check for button, if so stop motor push
                }
                myBlade.motorOff();
                state = WAIT;
                nextState = LOWER;  //HEADRAISE
                longTime = 0;
            }

            break;

        case CHECKLOWER:
            ////TODO: check if takes too long
            if (myBlade.checkIfDown())
            {
                state = HEADLOWER;
            }
            break;

        case HEADRAISE:

            break;

        case HEADLOWER:
            ////Lower head
            //if lowered
            state = CHECKHDLOWER;
            break;

        case CHECKHDRAISE:

            break;

        case CHECKHDLOWER:
            //if LOWERED
            state = WAIT;
            nextState = RAISE;
            break;

        case ERROR:
            LED_ON();
            myBlade.motorOff();
            myBlade.lowerBlade();
            myHead.motorOff();
            myHead.tiltOff();
            //Body off
            //Strobe off
            //Music off

            break;
        }

    }

}
ISR (TIMER2_COMPA_vect)
{
    localTimer++;
}
