
#define DEBUG

#include <avr/io.h>
#include <avr/interrupt.h>
#include <debug/debugserial.h>
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

//SET UP GLOBAL VARIABLES
volatile uint16_t localTimer; //VARIABLE TO HOLD CURRENT TIMER TICK
volatile uint16_t *ptr_Timer = &localTimer;  //POINTER TO ABOVE, ABLE TO PASS TO CLASS INSTANCES

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
    //SET UP GP TIMER
    TCCR2A = _BV(WGM21); //CTC MODE
    TCCR2B = _BV(CS20) | _BV(CS21) | _BV(CS22); // /1024 prescale
    TIMSK2 = _BV(OCIE2A); // CTCIE
    OCR2A = 155;  //100.16026 Hz (~10mS)

    //SET UP STATUS LED
    CONFIG_LED();

    /////////////////////////////////////////
    //SET UP STATE ENUMS
    enum D_State {SERIAL_DEBUG, NORMAL};
    enum V_State {INIT, WAIT, RUN, ERROR};
    enum V_NextState {LOWER, HEADLOWER, BODYTWITCH, INTERMISSION1, RAISE, HEADRAISE, HEADTILT, LOWERTILTROD, \
    LOWERHOIST, INTERMISSION2};

    //SET UP TIMING CONSTANTS
    //*_LONG = 0.5 SECONDS
    //*_SHORT = 10mS
    const uint16_t CHOP_PAUSE_LONG = SETTIMERMIN(1); //pause between intermission and blade lower
    const uint16_t HEAD_DROP_PAUSE_LONG = 1; ///////////turn to short
    const uint16_t BODY_TWITCH_START_PAUSE_LONG = 0; //////////turn to short
    const uint16_t INTERMISSION1_PAUSE_LONG = SETTIMERMIN(1); //pause between reset and lower
    const uint16_t RAISE_PAUSE_LONG = SETTIMERSEC(30); //pause after blade lower to raise again
    const uint16_t HEAD_RAISE_PAUSE_LONG = SETTIMERSEC(15); //pause after blade raised to raise head
    const uint16_t HEAD_TILT_PAUSE_LONG = SETTIMERSEC(1); //pause after head raise to tilt head into lock
    const uint16_t LOWER_TILT_ROD_PAUSE_LONG = SETTIMERSEC(1); //pause between reset and lower
    const uint16_t LOWER_HOIST_PAUSE_LONG = 0; //pause between reset and lower
    const uint16_t INTERMISSION2_PAUSE_LONG = SETTIMERMIN(1); //pause between reset and lower


    //SET UP GUILLOTINE OBJECTS
    DebugSerial DBSerial = DebugSerial();
    Blade myBlade = Blade(ptr_Timer);
    Head myHead = Head(ptr_Timer);
    Button myButton = Button(ptr_Timer);
    Strobe myStrobe = Strobe(ptr_Timer);
    Body myBody = Body(ptr_Timer);
    //Music myMusic = Music();

    //SET UP LOCAL (MAIN SCOPED) VARIABLES
    D_State debugMode = NORMAL; //STARTS UP IN NORMAL MODE
    V_State state = INIT;       //INITIAL MAIN STATE
    V_NextState nextState = LOWER;  //HOLDS NEXT RUN STATE
    uint16_t longTime = 0;      //HOLDS SECONDS
    uint8_t forceNextState = 0; //FLAG THAT BUTTON PRESS WHILE IN WAIT STATE
    uint8_t pollState = 0;      //HOLDS POLLING INFO (WORKING, COMPLETE, ERROR)
    uint16_t nextWaitTime = 0;  //HOLDS PAUSE TO NEXT RUN STATE

    sei();

    if (myButton.read()) debugMode = SERIAL_DEBUG;

    #ifdef DEBUG
    debugMode = SERIAL_DEBUG;
    #endif // DEBUG

    while(1)
    {
        if (DBSerial.available())
        {
            if (DBSerial.read() == 'D') debugMode = SERIAL_DEBUG;
        }

        switch (debugMode)
        {
        case NORMAL:

            switch (state)
            {
        //INIT: SET GUILLOTINE TO DEFAULT POSITIONS
        //      BLADE UP, HEAD UP, HEAD TILT UP
            case INIT:
            {
                LED_OFF();
                if (myBlade.init() == Blade::ERROR)        //MOVE BLADE TO UP POSITION
                {
                    state = ERROR;
                    break;
                }


                else if (myHead.init() == Head::ERROR)   //MOVE HEAD TILT ROD TO HOME POSITION
                {
                    state = ERROR;
                    break;
                }

                else if (myBody.init() == Body::ERROR)    //MAKE SURE LEGS WORKING
                {
                    state = ERROR;
                    break;
                }
                myStrobe.init();
                state = WAIT;
                nextState = LOWER;
                nextWaitTime = CHOP_PAUSE_LONG;
                setTimer(); //SO WAIT STATE STARTS WITH TIMER AT 0
                break;
            }
            //WAIT: WAIT UNTIL TIME TO DO nextState
            //      IF BUTTON PRESS, MOVE TO NEXT STATE NOW

            case WAIT:
                if (getTimer() >= 50)
                {
                    longTime++;
                    setTimer();
                }
                forceNextState = myButton.read();
                if (longTime >= nextWaitTime || forceNextState)
                {
                    state = RUN;
                    longTime = 0;
                }
                break;
            case RUN:
            {
                switch (nextState)
                {
                case LOWER:
                    pollState = 0;
                    myBlade.lowerBladePoll(Blade::RESET);
                    while (pollState != Blade::COMPLETE || pollState != Blade::ERROR)
                    {
                        pollState = myBlade.raiseBladePoll();
                        if(myButton.read())
                        {
                            pollState = Blade::ERROR;
                            break;
                        }
                    }
                    if (pollState == Blade::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = HEADLOWER;
                        nextWaitTime = HEAD_DROP_PAUSE_LONG;
                    }
                    break;

                case HEADLOWER:
                    pollState = 0;
                    myHead.lowerHeadPoll(Head::RESET);
                    while (pollState != Head::COMPLETE || pollState != Head::ERROR)
                    {
                        pollState = myHead.lowerHeadPoll();
                        if(myButton.read())
                        {
                            pollState = Head::ERROR;
                            break;
                        }
                    }
                    if (pollState == Head::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = BODYTWITCH;
                        nextWaitTime = BODY_TWITCH_START_PAUSE_LONG;
                    }
                    break;

                case BODYTWITCH:
                    pollState = 0;
                    myBody.bodyKickPoll(Body::RESET);
                    while (pollState != Body::COMPLETE || pollState != Body::ERROR)
                    {
                        pollState = myBody.bodyKickPoll();
                        if(myButton.read())
                        {
                            pollState = Body::ERROR;
                            break;
                        }
                    }
                    if (pollState == Body::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = INTERMISSION1;
                        nextWaitTime = INTERMISSION1_PAUSE_LONG;
                    }
                    break;

                case INTERMISSION1:
                    //while do something during intermissioon

                    state = WAIT;
                    nextState = RAISE;
                    nextWaitTime = RAISE_PAUSE_LONG;
                    break;

                case RAISE:
                    pollState = 0;
                    myBlade.raiseBladePoll(Blade::RESET);
                    while (pollState != Blade::COMPLETE || pollState != Blade::ERROR)
                    {
                        pollState = myBlade.raiseBladePoll();
                        if(myButton.read())
                        {
                            pollState = Head::ERROR;
                            break;
                        }
                    }
                    if (pollState == Blade::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = HEADRAISE;
                        nextWaitTime = HEAD_RAISE_PAUSE_LONG;
                    }
                    break;

                case HEADRAISE:
                    pollState = 0;
                    myHead.raiseHeadPoll(Head::RESET);
                    while (pollState != Head::COMPLETE || pollState != Head::ERROR)
                    {
                        pollState = myHead.raiseHeadPoll();
                        if(myButton.read())
                        {
                            pollState = Head::ERROR;
                            break;
                        }
                    }
                    if (pollState == Head::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = HEADTILT;
                        nextWaitTime = HEAD_TILT_PAUSE_LONG;
                    }
                    break;

                case HEADTILT:
                    pollState = 0;
                    myHead.tiltHeadUpPoll(Head::RESET);
                    while (pollState != Head::COMPLETE || pollState != Head::ERROR)
                    {
                        pollState = myHead.tiltHeadUpPoll();
                        if(myButton.read())
                        {
                            pollState = Head::ERROR;
                            break;
                        }
                    }
                    if (pollState == Head::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = LOWERTILTROD;
                        nextWaitTime = LOWER_TILT_ROD_PAUSE_LONG;
                    }
                    break;

                case LOWERTILTROD:
                    pollState = 0;
                    myHead.tiltRodDownPoll(Head::RESET);
                    while (pollState != Head::COMPLETE || pollState != Head::ERROR)
                    {
                        pollState = myHead.tiltRodDownPoll();
                        if(myButton.read())
                        {
                            pollState = Head::ERROR;
                            break;
                        }
                    }
                    if (pollState == Head::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = LOWERHOIST;
                        nextWaitTime = LOWER_HOIST_PAUSE_LONG;
                    }
                    break;

                case LOWERHOIST:
                    pollState = 0;
                    myHead.lowerHoistPoll(Head::RESET);
                    while (pollState != Head::COMPLETE || pollState != Head::ERROR)
                    {
                        pollState = myHead.lowerHoistPoll();
                        if(myButton.read())
                        {
                            pollState = Head::ERROR;
                            break;
                        }
                    }
                    if (pollState == Head::ERROR) state = ERROR;

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        state = WAIT;
                        nextState = INTERMISSION2;
                        nextWaitTime = INTERMISSION2_PAUSE_LONG;
                    }
                    break;

                case INTERMISSION2:
                    //while do something during intermissioon

                    state = WAIT;
                    nextState = LOWER;
                    nextWaitTime = CHOP_PAUSE_LONG;
                    break;
                }
                break;
            }
            case ERROR:
                LED_ON();
                myBlade.turnOff();
                myHead.turnOff();
                myBody.turnOff();
                myStrobe.strobeOff();
                //Music off
                if (myButton.read()) state = INIT;
                break;
        }
            break;

        case SERIAL_DEBUG:
            DBSerial.println(const_cast <char*> ("[--DBMode--]"));

            myBlade.printDebug(&DBSerial);
            myHead.printDebug(&DBSerial);
            myBody.printDebug(&DBSerial);

            while (debugMode == SERIAL_DEBUG || !myButton.read())
            {
                if (DBSerial.available())
                {
                    switch (DBSerial.read())
                    {
                    case 'd':
                        debugMode = NORMAL;
                        break;

                    case 'e':
                        myBlade.printDebug(&DBSerial);
                        myHead.printDebug(&DBSerial);
                        myBody.printDebug(&DBSerial);
                        break;

                    case 32: //SPACEBAR
                        DBSerial.println(const_cast <char*> ("[B]lade up / [b]lade down"));
                        DBSerial.println(const_cast <char*> ("[M]ag on / [m]ag off"));
                        DBSerial.println(const_cast <char*> ("[H]ead up / [h]ead down"));
                        DBSerial.println(const_cast <char*> ("h[O]ist down"));
                        DBSerial.println(const_cast <char*> ("[T]ilt up / [t]ilt down"));
                        DBSerial.println(const_cast <char*> ("head [R]elease on / [r]elease off"));
                        DBSerial.println(const_cast <char*> ("bod[Y] on / bod[y] off"));
                        DBSerial.println(const_cast <char*> ("[S]trobe on / [s]trobe off"));
                        DBSerial.println(const_cast <char*> ("[L]ed on / [l]ed off"));
                        DBSerial.println(const_cast <char*> ("m[U]sic on / m[u]sic off"));
                        DBSerial.println(const_cast <char*> ("[N]ext track / [P]rev track"));
                        DBSerial.println(const_cast <char*> ("d[e]bug info"));

                        break;

                    case 'B':
                        DBSerial.print(const_cast <char*> ("Blade Up.."));
                        myBlade.raiseBladePoll(Blade::RESET);
                        while (pollState != Blade::COMPLETE || pollState != Blade::ERROR) pollState = myBlade.raiseBladePoll();
                        if (pollState == Blade::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'b':
                        DBSerial.print(const_cast <char*> ("Blade Down.."));
                        myBlade.lowerBladePoll(Blade::RESET);
                        while (pollState != Blade::COMPLETE || pollState != Blade::ERROR) pollState = myBlade.lowerBladePoll();
                        if (pollState == Blade::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'M':
                        DBSerial.println(const_cast <char*> ("Mag On"));
                        myBlade.mag(1);
                        break;

                    case 'm':
                        DBSerial.println(const_cast <char*> ("Mag Off"));
                        myBlade.mag(0);
                        break;

                    case 'H':
                        DBSerial.print(const_cast <char*> ("Head Up.."));
                        myHead.raiseHeadPoll(Head::RESET);
                        while (pollState != Head::COMPLETE || pollState != Head::ERROR) pollState = myHead.raiseHeadPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'h':
                        DBSerial.print(const_cast <char*> ("Head Down.."));
                        myHead.lowerHeadPoll(Head::RESET);
                        while (pollState != Head::COMPLETE || pollState != Head::ERROR) pollState = myHead.lowerHeadPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'O':
                        DBSerial.print(const_cast <char*> ("Hoist Down.."));
                        myHead.lowerHoistPoll(Head::RESET);
                        while (pollState != Head::COMPLETE || pollState != Head::ERROR) pollState = myHead.lowerHoistPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'T':
                        DBSerial.print(const_cast <char*> ("Tilt Up.."));
                        myHead.tiltHeadUpPoll(Head::RESET);
                        while (pollState != Head::COMPLETE || pollState != Head::ERROR) pollState = myHead.tiltHeadUpPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 't':
                        DBSerial.print(const_cast <char*> ("Tilt Down.."));
                        myHead.tiltRodDownPoll(Head::RESET);
                        while (pollState != Head::COMPLETE || pollState != Head::ERROR) pollState = myHead.tiltRodDownPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'R':
                        DBSerial.print(const_cast <char*> ("Solenoid On"));
                        myHead.sol(1);
                        break;

                    case 'r':
                        DBSerial.print(const_cast <char*> ("Solenoid Off"));
                        myHead.sol(0);
                        break;

                    case 'Y':
                        DBSerial.print(const_cast <char*> ("Body On.."));
                        if (myBody.bodyOn()) DBSerial.println(const_cast <char*> ("Error"));
                        else DBSerial.println(const_cast <char*> ("Good"));
                        break;

                    case 'y':
                        DBSerial.print(const_cast <char*> ("Body Off.."));
                        if (myBody.bodyOn(0)) DBSerial.println(const_cast <char*> ("Error"));
                        else DBSerial.println(const_cast <char*> ("Good"));
                        myBody.turnOff();
                        break;

                    case 'S':
                        DBSerial.print(const_cast <char*> ("Strobe On"));
                        myStrobe.strobeOn();
                        break;

                    case 's':
                        DBSerial.print(const_cast <char*> ("Strobe Off"));
                        myStrobe.strobeOff();
                        break;

                    case 'L':
                        DBSerial.print(const_cast <char*> ("LED On"));
                        LED_ON();
                        break;

                    case 'l':
                        DBSerial.print(const_cast <char*> ("LED Off"));
                        LED_OFF();
                        break;
                    }
                }
            }
            state = INIT;
            break;
        }
    }
}
ISR (TIMER2_COMPA_vect)
{
    localTimer++;
}
