#include "head.h"

Head::Head(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T)
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
////init() return 0: error, 1: moved up, 2: already up and hoist already down
uint8_t Head::init(uint8_t mode)
{
    uint8_t ret = 0;

    ////INITIALIZE TILT MECH TO HOME; return 0 on error, 1 moved to home, 2 already home
    if (mode == 0)
    {
        if (IS_TILT_DOWN()) return 2;
        else
        {
            tiltHeadDown();
            setTimer();
            while (!IS_TILT_DOWN())
            {
                if (getTimer() > TILT_DOWN_MAX_TIME)
                {
                    tiltOff();
                    return 0;
                }
            }
            tiltOff();
            return 1;
        }
    }

    else if (mode == 1)
    {
        ////CHECK IF HEAD UP
        if (IS_HEAD_UP())
        {
            ////HEAD UP, CHECK IF TILTED UP
            if (IS_TILT_UP()) ret = 2;
            ////HEAD UP, TILT IS DOWN, MOVE TILT UP
            else
            {
                tiltHeadUp();
                setTimer();
                while (!IS_TILT_UP())
                {
                    if (getTimer() > TILT_UP_MAX_TIME)
                    {
                        tiltOff();
                        return 0;
                    }
                }
                tiltOff();
                ret = 1;
            }
        }
        ////HEAD DOWN, MOVE HEAD UP
        else
        {
            raiseHead();
            setTimer();
            while (!IS_HEAD_UP())
            {
                if (getTimer() > HEAD_UP_MAX_TIME || IS_HOIST_FAULT())
                {
                    motorOff();
                    return 0;
                }
            }
            motorOff();
            ////HEAD UP, MOVE TILT UP
            tiltHeadUp();
            setTimer();
            while (!IS_TILT_UP())
            {
                if (getTimer() > TILT_UP_MAX_TIME)
                {
                    tiltOff();
                    return 0;
                }
            }
            tiltOff();
            ret = 1;
        }
        if (IS_HOIST_DOWN()) ret = 2;
        else
        {
            lowerHoist();
            setTimer();
            while (!IS_HOIST_DOWN())
            {
                if (getTimer() > HOIST_DOWN_MAX_TIME || IS_HOIST_FAULT())
                {
                    motorOff();
                    return 0;
                }
            }
            motorOff();
            ret = 1;
        }
        return ret;
    }
}
void Head::raiseHead()
{
    OCR1A = 100;
    DIR_FORWARD();
    HOIST_ON();
}
void Head::lowerHead()
{
    setTimer();
    RELEASE_ON();
    while (getTimer() < 50);
    RELEASE_OFF();
}
void Head::lowerHoist()
{
    OCR1A = 200;
    DIR_REVERSE();
    HOIST_ON();
}
void Head::motorOff()
{
    OCR1A = 0;
    HOIST_OFF();
}
void Head::tiltHeadUp()
{
    TILT_UP();
    TILT_ON();
}
void Head::tiltHeadDown()
{
    TILT_DOWN();
    TILT_ON();
}
void Head::tiltOff()
{
    TILT_OFF();
}
uint8_t Head::checkIfUp()
{
    if (IS_HEAD_UP()) return 1;
    else return 0;
}
uint8_t Head::checkIfDown()
{
    if (IS_HEAD_DOWN()) return 1;
    else return 0;
}
uint8_t Head::checkIfHoistDown()
{
    if (IS_HOIST_DOWN()) return 1;
    else return 0;
}
uint8_t Head::checkIfTiltUp()
{
    if (IS_TILT_UP()) return 1;
    else return 0;
}
uint8_t Head::checkIfTiltDown()
{
    if (IS_TILT_DOWN()) return 1;
    else return 0;
}
