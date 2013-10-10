#include "body.h"

Body::Body(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T)
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
        bodyOn(DEFAULT_DUTY, 0);
        while (getTimer() < (BODY_INIT_LENGTH / 2))
        {

        }
        setTimer();
        bodyOff();
        while (getTimer() < 50)
        {

        }
    }
    return bodyOff();
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
    return !IS_BODY_FAULT();
}

uint8_t Body::bodyOff()
{
    BODY_OFF();
    OCR3A = 0;
    return !IS_BODY_FAULT();
}

uint8_t Body::bodyReverse()
{
    BODY_OFF();
    REVERSE_DIR_IN_STEP();
    BODY_ON();
}
