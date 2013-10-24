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
    CONFIG_TILT_SNSRS();
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
    TCCR0A = _BV(COM0A1) | _BV(WGM00);   //Phase Correct PWM
    TCCR0B = _BV(CS01) | _BV(CS00);     //Prescale 64, 490Hz
    OCR0A = 0;
}

uint8_t Head::init()
{
    uint8_t pollState = WORKING;

    tiltRodDownPoll(RESET);
    while (pollState != COMPLETE && pollState != ERROR) pollState = tiltRodDownPoll();
    if (pollState == ERROR) return pollState;

    pollState = WORKING;
    raiseHeadPoll(RESET);
    while (pollState != COMPLETE && pollState != ERROR) pollState = raiseHeadPoll();
    if (pollState == ERROR) return pollState;

    pollState = WORKING;
    tiltHeadUpPoll(RESET);
    while (pollState != COMPLETE && pollState != ERROR) pollState = tiltHeadUpPoll();
    if (pollState == ERROR) return pollState;

    pollState = WORKING;
    tiltRodDownPoll(RESET);
    while (pollState != COMPLETE && pollState != ERROR) pollState = tiltRodDownPoll();
    if (pollState == ERROR) return pollState;

    pollState = WORKING;
    lowerHoistPoll(RESET);
    while (pollState != COMPLETE && pollState != ERROR) pollState = lowerHoistPoll();
    if (pollState == ERROR) return pollState;

    return pollState;
}
uint8_t Head::raiseHeadPoll(uint8_t reset)
{
    if (reset == RESET) status = NOT_STARTED;

    switch (status)
    {
    case NOT_STARTED: //first time polled
        if (IS_HEAD_UP() || IS_TILT_UP()) status = DONE;
        else
        {
            OCR1A = 210;
            DIR_FORWARD();
            HOIST_ON();
            setTimer();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_HEAD_UP()) status = DONE;
        else if (getTimer() > HEAD_UP_MAX_TIME)
        {
            status = ERROR;
            turnOff();
        }
        break;
    case DONE:
        HOIST_OFF();
        OCR1A = 0;
        status = COMPLETE;
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
        else if (getTimer() > HEAD_DOWN_MAX_TIME)
        {
            status = ERROR;
            turnOff();
        }
        break;
    case DONE:
        status = COMPLETE;
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
            setTimer();
            OCR1A = 75;
            DIR_REVERSE();
            HOIST_ON();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_HOIST_DOWN()) status = DONE;
        else if (getTimer() > HOIST_DOWN_MAX_TIME)
        {
            status = ERROR;
            turnOff();
        }
        break;
    case DONE:
        HOIST_OFF();
        OCR1A = 0;
        status = COMPLETE;
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
        else if (!IS_HEAD_UP()) status = ERROR;
        else
        {
            setTimer();
            TILT_UP();
            TILT_ON();
            status = WORKING;
        }
        break;
    case WORKING:
        if (IS_TILT_UP()) status = DONE;
        else if (getTimer() > TILT_UP_MAX_TIME)
        {
            status = ERROR;
            turnOff();
        }
        break;
    case DONE:
        TILT_BRAKE();
        setTimer();
        while (getTimer() < TILT_MTR_BRAKE_HOLD);
        TILT_OFF();
        status = COMPLETE;
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
            setTimer();
            TILT_DOWN();
            TILT_ON();
            status = WORKING;

        }
        break;
    case WORKING:
        if (IS_TILT_DOWN()) status = DONE;
        else if (getTimer() > TILT_UP_MAX_TIME)
        {
            status = ERROR;
            turnOff();
        }
        break;
    case DONE:
        TILT_BRAKE();
        setTimer();
        while (getTimer() < TILT_MTR_BRAKE_HOLD);
        TILT_OFF();
        status = COMPLETE;
        break;
    }
    return status;
}

void Head::printDebug(DebugSerial *dbSerial)
{
    dbSerial->print(const_cast <char*>("\n\rHead: "));
    if (IS_HEAD_UP()) dbSerial->print(const_cast <char*>("UP"));
    else if (IS_HEAD_DOWN()) dbSerial->print(const_cast <char*>("DN"));
    else dbSerial->print(const_cast <char*>("LIMBO"));

    dbSerial->print(const_cast <char*>("\n\rRod: "));
    if (IS_TILT_DOWN()) dbSerial->print(const_cast <char*>("DN"));
    else dbSerial->print(const_cast <char*>("UP"));

    dbSerial->print(const_cast <char*>("\n\rTilt: "));
    if (IS_TILT_UP()) dbSerial->print(const_cast <char*>("UP"));
    else dbSerial->print(const_cast <char*>("DN"));

    dbSerial->print(const_cast <char*>("\n\rHoist: "));
    if (IS_HOIST_DOWN()) dbSerial->print(const_cast <char*>("DN"));
    else dbSerial->print(const_cast <char*>("UP"));

    dbSerial->print(const_cast <char*>("\n\rMotor Fault:"));
    if (IS_HOIST_FAULT()) dbSerial->print(const_cast <char*>(" Y"));
    else dbSerial->print(const_cast <char*>(" N"));
}

void Head::sol(uint8_t state)
{
    if (state) RELEASE_ON();
    else RELEASE_OFF();
}
