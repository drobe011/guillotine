
#define DEBUG

#include "gulliotine.h"

//SET UP GLOBAL VARIABLES
volatile uint16_t localTimer; //VARIABLE TO HOLD CURRENT TIMER TICK (10mS)
volatile uint16_t *ptr_Timer = &localTimer;  //POINTER TO ABOVE, ABLE TO PASS TO CLASS INSTANCES

int main(void)
{
    //TURN OFF POWER TO THINGS I DON'T NEED
    ADCSRA = 0; //DISABLE ADC
    PRR0 = _BV(PRTWI) | _BV(PRADC) | _BV(PRSPI); //TURN OFF POWER TO ADC, TWI, SPI

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
    const uint16_t CHOP_PAUSE_LONG = SETTIMERSEC(20); //pause between intermission and blade lower
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
    DebugSerial DBSerial = DebugSerial(115200);
    MusicSerial myMusic = MusicSerial(38400);
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

    #ifdef SND_STARTUP
        myMusic.playHold(SND_STARTUP, &myButton);
    #endif // SND_STARTUP

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
                #ifdef SND_ENTERINIT
                    myMusic.playHold(SND_ENTERINIT, &myButton);
                #endif // SND_ENTERINIT
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

////                else if (myBody.init() == Body::ERROR)    //MAKE SURE LEGS WORKING
////                {
////                    state = ERROR;
////                    break;
////                }
                myStrobe.init();
                #ifdef SND_LEAVEINIT
                    myMusic.playHold(SND_LEAVEINIT, &myButton);
                #endif // SND_LEAVEINIT
                state = WAIT;
                nextState = LOWER;
                nextWaitTime = CHOP_PAUSE_LONG;
                setTimer(); //SO WAIT STATE STARTS WITH TIMER AT 0
                #ifdef SND_LOWER_BEFORE
                    myMusic.trigger(SND_LOWER_BEFORE);
                #endif // SND_LOWER_BEFORE

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
                    myMusic.stop();
                }
                break;
            case RUN:
            {
                switch (nextState)
                {
                case LOWER:
                    #ifdef SND_LOWER_ON
                        myMusic.trigger(SND_LOWER_ON);
                    #endif // SND_LOWER_ON
                    pollState = Blade::WORKING;
                    myBlade.lowerBladePoll(Blade::RESET);
                    while (pollState != Blade::COMPLETE && pollState != Blade::ERROR)
                    {
                        pollState = myBlade.lowerBladePoll();
                        if(myButton.read())
                        {
                            pollState = Blade::ERROR;
                            break;
                        }
                    }
                    if (pollState == Blade::ERROR)
                    {
                        state = ERROR;
                    #ifdef DEBUG
                        DBSerial.println(const_cast <char*> ("LOWERERROR"));
                    #endif // DEBUG
                    }

                    ////GOTO NEXT STATE AND SETUP NEXT EVENT
                    else
                    {
                        #ifdef SND_LOWER_AFTER
                            myMusic.trigger(SND_LOWER_AFTER);
                        #endif // SND_LOWER_AFTER
                        state = WAIT;
                        nextState = HEADLOWER;
                        nextWaitTime = HEAD_DROP_PAUSE_LONG;
                        #ifdef SND_HEAD_LOWER_BEFORE
                            myMusic.trigger(SND_HEAD_LOWER_BEFORE);
                        #endif // SND_HEAD_LOWER_BEFORE
                    }
                    break;

                case HEADLOWER:
                    #ifdef SND_HEAD_LOWER_ON
                        myMusic.trigger(SND_HEAD_LOWER_ON);
                    #endif // SND_HEAD_LOWER_ON
                    pollState = Head::WORKING;
                    myHead.lowerHeadPoll(Head::RESET);
                    while (pollState != Head::COMPLETE && pollState != Head::ERROR)
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
                        #ifdef SND_HEAD_LOWER_AFTER
                            myMusic.trigger(SND_HEAD_LOWER_AFTER);
                        #endif // SND_HEAD_LOWER_AFTER
                        state = WAIT;
                        nextState = BODYTWITCH;
                        nextWaitTime = BODY_TWITCH_START_PAUSE_LONG;
                        #ifdef SND_BODY_BEFORE
                            myMusic.trigger(SND_BODY_BEFORE);
                        #endif // SND_BODY_BEFORE
                    }
                    break;

                case BODYTWITCH:
                    #ifdef SND_BODY_ON
                        myMusic.trigger(SND_BODY_ON);
                    #endif // SND_BODY_ON
                    pollState = Body::WORKING;
                    myBody.bodyKickPoll(Body::RESET);
                    while (pollState != Body::COMPLETE && pollState != Body::ERROR)
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
                        #ifdef SND_BODY_AFTER
                            myMusic.trigger(SND_BODY_AFTER);
                        #endif // SND_BODY_AFTER
                        state = WAIT;
                        nextState = INTERMISSION1;
                        nextWaitTime = INTERMISSION1_PAUSE_LONG;
                        #ifdef SND_INT1_BEFORE
                            myMusic.trigger(SND_INT1_BEFORE);
                        #endif // SND_INT1_BEFORE
                    }
                    break;

                case INTERMISSION1:
                    #ifdef SND_INT1_ON
                        myMusic.trigger(SND_INT1_ON);
                    #endif // SND_INT1_ON

                    //while do something during intermissioon

                    #ifdef SND_INT1_AFTER
                        myMusic.trigger(SND_INT1_AFTER);
                    #endif // SND_INT1_AFTER

                    state = WAIT;
                    nextState = RAISE;
                    nextWaitTime = RAISE_PAUSE_LONG;
                    #ifdef SND_RAISE_BEFORE
                        myMusic.trigger(SND_RAISE_BEFORE);
                    #endif // SND_RAISE_BEFORE
                    break;

                case RAISE:
                    #ifdef SND_RAISE_ON
                        myMusic.trigger(SND_RAISE_ON);
                    #endif // SND_RAISE_ON
                    pollState = Blade::WORKING;
                    myBlade.raiseBladePoll(Blade::RESET);
                    while (pollState != Blade::COMPLETE && pollState != Blade::ERROR)
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
                        #ifdef SND_RAISE_AFTER
                            myMusic.trigger(SND_RAISE_AFTER);
                        #endif // SND_RAISE_AFTER
                        state = WAIT;
                        nextState = HEADRAISE;
                        nextWaitTime = HEAD_RAISE_PAUSE_LONG;
                        #ifdef SND_HEAD_RAISE_BEFORE
                            myMusic.trigger(SND_HEAD_RAISE_BEFORE);
                        #endif // SND_HEAD_RAISE_BEFORE
                    }
                    break;

                case HEADRAISE:
                    #ifdef SND_HEAD_RAISE_ON
                        myMusic.trigger(SND_HEAD_RAISE_ON);
                    #endif // SND_HEAD_RAISE_ON
                    pollState = Head::WORKING;
                    myHead.raiseHeadPoll(Head::RESET);
                    while (pollState != Head::COMPLETE && pollState != Head::ERROR)
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
                        #ifdef SND_HEAD_RAISE_AFTER
                            myMusic.trigger(SND_HEAD_RAISE_AFTER);
                        #endif // SND_HEAD_RAISE_AFTER
                        state = WAIT;
                        nextState = HEADTILT;
                        nextWaitTime = HEAD_TILT_PAUSE_LONG;
                        #ifdef SND_HEAD_TILT_BEFORE
                            myMusic.trigger(SND_HEAD_TILT_BEFORE);
                        #endif // SND_HEAD_TILT_BEFORE
                    }
                    break;

                case HEADTILT:
                    #ifdef SND_HEAD_TILT_ON
                        myMusic.trigger(SND_HEAD_TILT_ON);
                    #endif // SND_HEAD_TILT_ON
                    pollState = Head::WORKING;
                    myHead.tiltHeadUpPoll(Head::RESET);
                    while (pollState != Head::COMPLETE && pollState != Head::ERROR)
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
                        #ifdef SND_HEAD_TILT_AFTER
                            myMusic.trigger(SND_HEAD_TILT_AFTER);
                        #endif // SND_HEAD_TILT_AFTER
                        state = WAIT;
                        nextState = LOWERTILTROD;
                        nextWaitTime = LOWER_TILT_ROD_PAUSE_LONG;
                        #ifdef SND_LOWER_TILT_BEFORE
                            myMusic.trigger(SND_LOWER_TILT_BEFORE);
                        #endif // SND_LOWER_TILT_BEFORE
                    }
                    break;

                case LOWERTILTROD:
                    #ifdef SND_LOWER_TILT_ON
                        myMusic.trigger(SND_LOWER_TILT_ON);
                    #endif // SND_LOWER_TILT_ON
                    pollState = Head::WORKING;
                    myHead.tiltRodDownPoll(Head::RESET);
                    while (pollState != Head::COMPLETE && pollState != Head::ERROR)
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
                        #ifdef SND_LOWER_TILT_AFTER
                            myMusic.trigger(SND_LOWER_TILT_AFTER);
                        #endif // SND_LOWER_TILT_AFTER
                        state = WAIT;
                        nextState = LOWERHOIST;
                        nextWaitTime = LOWER_HOIST_PAUSE_LONG;
                        #ifdef SND_LOWER_HOIST_BEFORE
                            myMusic.trigger(SND_LOWER_HOIST_BEFORE);
                        #endif // SND_LOWER_HOIST_BEFORE
                    }
                    break;

                case LOWERHOIST:
                    #ifdef SND_LOWER_HOIST_ON
                        myMusic.trigger(SND_LOWER_HOIST_ON);
                    #endif // SND_LOWER_HOIST_ON
                    pollState = Head::WORKING;
                    myHead.lowerHoistPoll(Head::RESET);
                    while (pollState != Head::COMPLETE && pollState != Head::ERROR)
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
                        #ifdef SND_LOWER_HOIST_AFTER
                            myMusic.trigger(SND_LOWER_HOIST_AFTER);
                        #endif // SND_LOWER_HOIST_AFTER
                        state = WAIT;
                        nextState = INTERMISSION2;
                        nextWaitTime = INTERMISSION2_PAUSE_LONG;
                        #ifdef SND_INT2_BEFORE
                            myMusic.trigger(SND_INT2_BEFORE);
                        #endif // SND_INT2_BEFORE
                    }
                    break;

                case INTERMISSION2:
                    #ifdef SND_INT2_ON
                        myMusic.trigger(SND_INT2_ON);
                    #endif // SND_INT2_ON

                    //while do something during intermissioon

                    #ifdef SND_INT2_AFTER
                        myMusic.trigger(SND_INT2_AFTER);
                    #endif // SND_INT2_AFTER

                    state = WAIT;
                    nextState = LOWER;
                    nextWaitTime = CHOP_PAUSE_LONG;
                    #ifdef SND_LOWER_BEFORE
                        myMusic.trigger(SND_LOWER_BEFORE);
                    #endif // SND_LOWER_BEFORE
                    break;
                }
                break;
            }
            case ERROR:
                LED_ON();
                myMusic.stop();
                myBlade.turnOff();
                myHead.turnOff();
                myBody.turnOff();
                myStrobe.strobeOff();

                #ifdef SND_ENTERERROR
                    myMusic.playHold(SND_ENTERERROR, &myButton);
                #endif // SND_ENTERERROR
                uint8_t checkSerial = 0;

                while (!myButton.read())
                {
                    if (DBSerial.available())
                    {
                        if (DBSerial.read() == 'D') break;
                    }
                }
                state = INIT;
                #ifdef SND_LEAVEERROR
                    myMusic.playHold(SND_LEAVEERROR, &myButton);
                #endif // SND_LEAVEERROR
                break;
        }
            break;

        case SERIAL_DEBUG:
            LED_OFF();
            #ifdef SND_ENTERDEBUG
                myMusic.playHold(SND_ENTERDEBUG, &myButton);
            #endif // SND_ENTERDEBUG

            DBSerial.println(const_cast <char*> ("\n\r[--DBMode--]"));

            myBlade.printDebug(&DBSerial);
            myHead.printDebug(&DBSerial);
            myBody.printDebug(&DBSerial);
            DBSerial.println(const_cast <char*> ("\n\r---"));

            while (debugMode == SERIAL_DEBUG && !myButton.read())
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
                        DBSerial.println(const_cast <char*> ("\n\r---"));
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
                        DBSerial.println(const_cast <char*> ("m[U]sic on / off"));
                        DBSerial.println(const_cast <char*> ("[N]ext track / [P]rev track"));
                        DBSerial.println(const_cast <char*> ("d[e]bug info"));

                        break;

                    case 'B':
                        DBSerial.print(const_cast <char*> ("Blade Up.."));
                        pollState = Blade::WORKING;
                        myBlade.raiseBladePoll(Blade::RESET);
                        while (pollState != Blade::COMPLETE && pollState != Blade::ERROR) pollState = myBlade.raiseBladePoll();
                        if (pollState == Blade::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'b':
                        DBSerial.print(const_cast <char*> ("Blade Down.."));
                        pollState = Blade::WORKING;
                        myBlade.lowerBladePoll(Blade::RESET);
                        while (pollState != Blade::COMPLETE && pollState != Blade::ERROR) pollState = myBlade.lowerBladePoll();
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
                        pollState = Head::WORKING;
                        myHead.raiseHeadPoll(Head::RESET);
                        while (pollState != Head::COMPLETE && pollState != Head::ERROR) pollState = myHead.raiseHeadPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'h':
                        DBSerial.print(const_cast <char*> ("Head Down.."));
                        pollState = Head::WORKING;
                        myHead.lowerHeadPoll(Head::RESET);
                        while (pollState != Head::COMPLETE && pollState != Head::ERROR) pollState = myHead.lowerHeadPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'O':
                        DBSerial.print(const_cast <char*> ("Hoist Down.."));
                        pollState = Head::WORKING;
                        myHead.lowerHoistPoll(Head::RESET);
                        while (pollState != Head::COMPLETE && pollState != Head::ERROR) pollState = myHead.lowerHoistPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'T':
                        DBSerial.print(const_cast <char*> ("Tilt Up.."));
                        pollState = Head::WORKING;
                        myHead.tiltHeadUpPoll(Head::RESET);
                        while (pollState != Head::COMPLETE && pollState != Head::ERROR) pollState = myHead.tiltHeadUpPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 't':
                        DBSerial.print(const_cast <char*> ("Tilt Down.."));
                        pollState = Head::WORKING;
                        myHead.tiltRodDownPoll(Head::RESET);
                        while (pollState != Head::COMPLETE && pollState != Head::ERROR) pollState = myHead.tiltRodDownPoll();
                        if (pollState == Head::COMPLETE) DBSerial.println(const_cast <char*> ("Good"));
                        else DBSerial.println(const_cast <char*> ("Error"));
                        break;

                    case 'R':
                        DBSerial.println(const_cast <char*> ("Solenoid On"));
                        myHead.sol(1);
                        break;

                    case 'r':
                        DBSerial.println(const_cast <char*> ("Solenoid Off"));
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
                        DBSerial.println(const_cast <char*> ("Strobe On"));
                        myStrobe.strobeOn();
                        break;

                    case 's':
                        DBSerial.println(const_cast <char*> ("Strobe Off"));
                        myStrobe.strobeOff();
                        break;

                    case 'L':
                        DBSerial.println(const_cast <char*> ("LED On"));
                        LED_ON();
                        break;

                    case 'l':
                        DBSerial.println(const_cast <char*> ("LED Off"));
                        LED_OFF();
                        break;

                    case 'U':
                        DBSerial.println(const_cast <char*> ("Music Toggle"));
                        myMusic.togglePlay();
                        break;

                    case 'N':
                        DBSerial.println(const_cast <char*> ("Next Track"));
                        myMusic.nextTrack();
                        break;

                    case 'P':
                        DBSerial.println(const_cast <char*> ("Prev Track"));
                        myMusic.prevTrack();
                        break;
                    }
                }
            }
            #ifdef SND_LEAVEDEBUG
                myMusic.playHold(SND_LEAVEDEBUG, &myButton);
            #endif // SND_LEAVEDEBUG
            state = INIT;
            debugMode = NORMAL;
            DBSerial.println(const_cast <char*> ("Exit"));
            break;
        }
    }
}
ISR (TIMER2_COMPA_vect)
{
    localTimer++;
}
