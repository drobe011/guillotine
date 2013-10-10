#ifndef BUTTON_H
#define BUTTON_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_PORT PINA
#define BUTTON_PIN 3


class Button
{
    public:
        Button(volatile uint16_t * ptr_T);
        uint8_t state;
        uint8_t read();
    protected:
    private:
        volatile uint16_t * ptr_Timer;
        __inline__ uint8_t IS_BUTTON_PRESSED()
        {
            return !(BUTTON_PORT & _BV(BUTTON_PIN));
        }
};

#endif // BUTTON_H
