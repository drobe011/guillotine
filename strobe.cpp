#include "strobe.h"

Strobe::Strobe(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T)
{
    CONFIG_STROBE();
}

void Strobe::init()
{
    STROBE_ON();
    setTimer();
    while (getTimer() < STROBE_INIT_LENGTH)
    {

    }
    STROBE_OFF();
}

void Strobe::strobeOn()
{
    STROBE_ON();
}

void Strobe::strobeOff()
{
    STROBE_OFF();
}
