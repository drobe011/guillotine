#include "blade.h"


Blade::Blade(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T), bladeStatus(NOT_STARTED)
{
    CONFIG_BLADE_UP();
    CONFIG_MAG();
}

void Blade::raiseBlade()
{
    BLADE_MOTOR_ON();
    BLADE_HOLD();
}

uint8_t Blade::lowerBladePoll(uint8_t reset)
{
    if (reset == RESET) bladeStatus = NOT_STARTED;

    switch (bladeStatus)
    {
    case NOT_STARTED: //first time polled
        if (IS_BLADE_DOWN()) bladeStatus = DONE;
        else
        {
            BLADE_RELEASE();
            setTimer();
            bladeStatus = WORKING;
        }
        break;
    case WORKING:
        if (IS_BLADE_DOWN()) bladeStatus = DONE;
        else if (getTimer() > BLADE_DOWN_MAX_TIME) bladeStatus = ERROR;
        break;
    case DONE:
        bladeStatus = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return bladeStatus;
}

void Blade::turnOff()
{
    BLADE_MOTOR_OFF();
    BLADE_RELEASE();
}

uint8_t Blade::init()
{
    uint8_t pollState = 0;

    raiseBladePoll(RESET);
    while (pollState != COMPLETE || pollState != ERROR) pollState = raiseBladePoll();
    return pollState;
}

uint8_t Blade::raiseBladePoll(uint8_t reset)
{
    if (reset == RESET) bladeStatus = NOT_STARTED;

    switch (bladeStatus)
    {
    case NOT_STARTED: //first time polled
        if (IS_BLADE_UP()) bladeStatus = DONE;
        else
        {
            raiseBlade();
            setTimer();
            bladeStatus = WORKING;
        }
        break;
    case WORKING:
        if (IS_BLADE_UP()) bladeStatus = DONE;
        else if (getTimer() > BLADE_UP_MAX_TIME) bladeStatus = ERROR;
        break;
    case DONE:
        setTimer();
        while (getTimer() < BLADE_MTR_PAUSE_TOP);
        BLADE_MOTOR_OFF();
        bladeStatus = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return bladeStatus;
}
void Blade::printDebug(DebugSerial *dbSerial)
{
    dbSerial->print(const_cast <char*>("Blade: "));
    if (IS_BLADE_UP()) dbSerial->println(const_cast <char*>("UP"));
    else if (IS_BLADE_DOWN()) dbSerial->println(const_cast <char*>("DN"));
}
void Blade::mag(uint8_t state)
{
    if (state) BLADE_HOLD();
    else BLADE_RELEASE();
}
