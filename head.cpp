#include "head.h"

Head::Head(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T), status(NOT_STARTED)
{
    CONFIG_HOIST();
    CONFIG_RELEASE();
    CONFIG_HOIST_ENABLE();
    CONFIG_DIR();
    CONFIG_TILT();
    CONFIG_TILT_EN();
    CONFIG_TILT_DIR();
    configHoistTimer();
    configTiltTimer();
}

void Head::configHoistTimer()
{
    //SETUP PWM TIMER
    TCCR1A = _BV(COM1A1) | _BV(WGM10);
    TCCR1B = _BV(CS12);
    OCR1A = 0;
}

void Head::configTiltTimer()
{
    TCCR0A = _BV(COM0A0) | _BV(WGM01);   //CTC Toggle
    TCCR0B = _BV(CS00) | _BV(CS02);     //1024  30hz with OCR0A=255
    OCR0A = 255;
}

uint8_t Head::init()
{
    uint8_t pollState = 0;

    tiltRodDownPoll(RESET);
    while (pollState != COMPLETE || pollState != ERROR) pollState = tiltRodDownPoll();
    if (pollState == ERROR) return pollState;

    raiseHeadPoll(RESET);
    while (pollState != COMPLETE || pollState != ERROR) pollState = raiseHeadPoll();
    if (pollState == ERROR) return pollState;

    tiltHeadUpPoll(RESET);
    while (pollState != COMPLETE || pollState != ERROR) pollState = tiltHeadUpPoll();
    if (pollState == ERROR) return pollState;

    tiltRodDownPoll(RESET);
    while (pollState != COMPLETE || pollState != ERROR) pollState = tiltRodDownPoll();
    if (pollState == ERROR) return pollState;

    lowerHoistPoll(RESET);
    while (pollState != COMPLETE || pollState != ERROR) pollState = lowerHoistPoll();
    if (pollState == ERROR) return pollState;

    return pollState;
}
uint8_t Head::raiseHeadPoll(uint8_t reset)
{
    if (reset == RESET) status = NOT_STARTED;

    switch (status)
    {
    case NOT_STARTED: //first time polled
        if (IS_HEAD_UP()) status = DONE;
        else
        {
            OCR1A = 100;
            DIR_FORWARD();
            HOIST_ON();
            setTimer();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_HEAD_UP()) status = DONE;
        else if (getTimer() > HEAD_UP_MAX_TIME) status = ERROR;
        break;
    case DONE:
        HOIST_OFF();
        OCR1A = 0;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return status;
}
uint8_t Head::lowerHeadPoll(uint8_t reset)
{
   if (reset == RESET) status = NOT_STARTED;

    switch (status)
    {
    case NOT_STARTED: //first time polled
        if (IS_HEAD_DOWN()) status = DONE;
        else
        {
            setTimer();
            RELEASE_ON();
            while (getTimer() < RELEASE_ON_MAX_TIME);
            RELEASE_OFF();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_HEAD_DOWN()) status = DONE;
        else if (getTimer() > HEAD_DOWN_MAX_TIME) status = ERROR;
        break;
    case DONE:
        status = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return status;
}
uint8_t Head::lowerHoistPoll(uint8_t reset)
{
   if (reset == RESET) status = NOT_STARTED;

    switch (status)
    {
    case NOT_STARTED: //first time polled
        if (IS_HOIST_DOWN()) status = DONE;
        else
        {
            OCR1A = 200;
            DIR_REVERSE();
            HOIST_ON();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_HOIST_DOWN()) status = DONE;
        else if (getTimer() > HOIST_DOWN_MAX_TIME) status = ERROR;
        break;
    case DONE:
        HOIST_OFF();
        OCR1A = 0;
        status = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return status;
}
void Head::turnOff()
{
    OCR1A = 0;
    HOIST_OFF();
    TILT_OFF();
}
uint8_t Head::tiltHeadUpPoll(uint8_t reset)
{
   if (reset == RESET) status = NOT_STARTED;

    switch (status)
    {
    case NOT_STARTED: //first time polled
        if (IS_TILT_UP()) status = DONE;
        else
        {
            TILT_UP();
            TILT_ON();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_TILT_UP()) status = DONE;
        else if (getTimer() > TILT_UP_MAX_TIME) status = ERROR;
        break;
    case DONE:
        TILT_OFF();
        status = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return status;
}
uint8_t Head::tiltRodDownPoll(uint8_t reset)
{
   if (reset == RESET) status = NOT_STARTED;

    switch (status)
    {
    case NOT_STARTED: //first time polled
        if (IS_TILT_DOWN()) status = DONE;
        else
        {
            TILT_DOWN();
            TILT_ON();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_TILT_DOWN()) status = DONE;
        else if (getTimer() > TILT_UP_MAX_TIME) status = ERROR;
        break;
    case DONE:
        TILT_OFF();
        status = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return status;
}

void Head::printDebug(DebugSerial *dbSerial)
{
    dbSerial->print(const_cast <char*>("Head: "));
    if (IS_HEAD_UP()) dbSerial->print(const_cast <char*>("UP, "));
    else if (IS_HEAD_DOWN()) dbSerial->print(const_cast <char*>("DN, "));

    dbSerial->print(const_cast <char*>("Tilt: "));
    if (IS_TILT_UP()) dbSerial->print(const_cast <char*>("UP, "));
    else if (IS_TILT_DOWN()) dbSerial->print(const_cast <char*>("DN, "));

    dbSerial->print(const_cast <char*>("Hoist: "));
    if (IS_HOIST_DOWN()) dbSerial->print(const_cast <char*>("DN, "));
    else dbSerial->print(const_cast <char*>("UP, "));

    dbSerial->print(const_cast <char*>("Motor Fault:"));
    if (IS_HOIST_FAULT()) dbSerial->println(const_cast <char*>(" Y"));
    else dbSerial->println(const_cast <char*>(" N"));
}

void Head::sol(uint8_t state)
{
    if (state) RELEASE_ON();
    else RELEASE_OFF();
}
