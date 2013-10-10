#include "blade.h"


Blade::Blade(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T)
{
    CONFIG_BLADE_UP();
    CONFIG_MAG();
}

void Blade::raiseBlade()
{
    BLADE_MOTOR_ON();
    BLADE_HOLD();
}

void Blade::lowerBlade()
{
    BLADE_RELEASE();
}

uint8_t Blade::checkIfUp()
{
    if (IS_BLADE_UP()) return 1;
    else return 0;
}

uint8_t Blade::checkIfDown()
{
    if (IS_BLADE_DOWN()) return 1;
    else return 0;
}

void Blade::motorOff()
{
    BLADE_MOTOR_OFF();
}
////init() return 0: error, 1: moved up, 2: already up
uint8_t Blade::init()
{
    if (IS_BLADE_UP()) return 2;
    else
    {
        raiseBlade();
        setTimer();
        while (!IS_BLADE_UP())
        {
            if (getTimer() > BLADE_UP_MAX_TIME)
            {
                BLADE_MOTOR_OFF();
                BLADE_RELEASE();
                return 0;
            }
        }
        setTimer();
        while (getTimer() < BLADE_MTR_PAUSE_TOP);
        BLADE_MOTOR_OFF();
        return 1;
    }
}
