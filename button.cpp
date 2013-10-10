#include "button.h"

Button::Button(volatile uint16_t * ptr_T) :
    ptr_Timer(ptr_T)
{
    //ctor
}

uint8_t Button::read()
{
    if (IS_BUTTON_PRESSED())
    {
        if (state == 0)
        {
            state = 1;
            return 1;
        }
        else return 0;
    }
    else
    {
        state = 0;
        return 0;
    }

}
