#include "body.h"

Body::Body(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T), bodyStatus(NOT_STARTED)
{
    CONFIG_BODY();
    CONFIG_ENABLE();
    CONFIG_DIR();
    configTimer();
}

uint8_t Body::init()
{
    for (uint8_t tempI = 0; tempI < 2; tempI++)
    {
        setTimer();
        bodyOn(DEFAULT_DUTY, tempI);
        while (getTimer() < (BODY_INIT_LENGTH / 2))
        {

        }
        setTimer();
        turnOff();
        while (getTimer() < 50)
        {

        }
    }
    return COMPLETE;
}

void Body::configTimer()
{
    //SETUP PWM TIMER
    TCCR3A = _BV(COM3A1) | _BV(WGM30);
    TCCR3B = _BV(CS32);
    OCR3A = 0;
}
//duty = 1-255 (0 is off); dir 1 = forward, 2 = reverse
uint8_t Body::bodyOn(uint8_t duty, uint8_t dir)
{
    OCR3A = duty;
    if (dir) DIR_FORWARD();
    else DIR_REVERSE();
    BODY_ON();
    return IS_BODY_FAULT();
}

void Body::turnOff()
{
    BODY_OFF();
    OCR3A = 0;
}

//uint8_t Body::bodyReverse()
//{
//    BODY_OFF();
//    REVERSE_DIR_IN_STEP();
//    BODY_ON();
//    return !IS_BODY_FAULT();
//}

uint8_t Body::bodyKickPoll(uint8_t reset)
{
    if (reset == RESET) bodyStatus = NOT_STARTED;

    switch (bodyStatus)
    {
    case NOT_STARTED: //first time polled
        bodyOn();
        setTimer();
        bodyStatus = WORKING;
        break;
    case WORKING:
        if (getTimer() > BODY_INIT_LENGTH || IS_BODY_FAULT()) bodyStatus = ERROR;
        break;
    case DONE:
        bodyStatus = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return bodyStatus;
}

void Body::printDebug(DebugSerial *dbSerial)
{
    dbSerial->print(const_cast <char*>("Body Fault:"));
    if (IS_BODY_FAULT()) dbSerial->println(const_cast <char*>(" Y"));
    else dbSerial->println(const_cast <char*>(" N"));
}
