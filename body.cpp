#include "body.h"

Body::Body(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T), bodyStatus(NOT_STARTED)
{
    CONFIG_BODY();
    CONFIG_ENABLE();
    CONFIG_DIR();
    CONFIG_POS();
    configTimer();
}

uint8_t Body::init()
{
    setTimer();
    bodyOn(DEFAULT_DUTY, 0);
    while (getTimer() < BODY_INIT_LENGTH)
    {

    }
    turnOff();
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
void Body::bodyOn(uint8_t duty, uint8_t dir)
{
    OCR3A = duty;
    if (dir) DIR_FORWARD();
    else DIR_REVERSE();
    BODY_ON();
}

void Body::turnOff(uint8_t notnow)
{
    if (notnow == 1)
    {
        setTimer();
        while (!IS_BODY_REST())
        {
            if (getTimer() > BODY_MAX_REST_WAIT) break;
        }
    }
    BODY_OFF();
    OCR3A = 0;
}

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
        if (getTimer() > BODY_KICK_LENGTH) bodyStatus = DONE;
        break;
    case DONE:
        turnOff();
        bodyStatus = COMPLETE;
        break;
    case ERROR:
        turnOff();
        break;
    }
    return bodyStatus;
}

void Body::bodyTwitch()
{
    uint8_t dirToggle = 0;
    for (uint8_t loops = 145; loops < 155; loops++)
    {
        bodyOn(loops+100, dirToggle);
        setTimer();
        while (getTimer() < 15);
        turnOff(2);
        dirToggle ^= 1;
        PINC |= _BV(PINC6);
    }
    turnOff(2);
}
void Body::printDebug(DebugSerial *dbSerial)
{
    dbSerial->print(const_cast <char*>("\n\rBody Position: "));
    if (IS_BODY_REST()) dbSerial->print(const_cast <char*>("REST"));
    else dbSerial->print(const_cast <char*>("KICK"));
    //dbSerial->println(const_cast <char*> ("how"));
}
