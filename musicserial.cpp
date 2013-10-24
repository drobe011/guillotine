#include "musicserial.h"
#include <avr/interrupt.h>
#include <string.h>
#include <math.h>

MusicSerial::MusicSerial()
{
    init(25); //38400 baud at 16Mhz
}

MusicSerial::MusicSerial(double UBd, double cpu)
{
    init(lrint(((cpu / (UBd * 16))) - 1));
    //init(25);
}

volatile MusicSerial::status MusicSerial::stat = MusicSerial::UNSET;

void MusicSerial::write(uint8_t byteToSend)
{
	while ((UCSR1A & _BV(UDRE1)) == 0)
    {

    }
	UDR1 = byteToSend;
}

void MusicSerial::togglePlay()
{
    MusicSerial::stat = PLAY;
    write('O');
}

void MusicSerial::stop()
{
    if (MusicSerial::stat == PLAY) write('O');
}

void MusicSerial::trigger(uint8_t track)
{
    MusicSerial::stat = PLAY;
    write('t');
    write(track);
}

uint8_t MusicSerial::playHold(uint8_t track, Button *btn) //send button object
{
    MusicSerial::stat = PLAY;
    write('t');
    write(track);
    while (MusicSerial::stat == PLAY && !btn->read())
    {
        //wait
    }
    if (MusicSerial::stat == PLAY) togglePlay();
    return MusicSerial::stat;
}
void MusicSerial::nextTrack()
{
    write('F');
}

void MusicSerial::prevTrack()
{
    write('R');
}

void MusicSerial::volume(uint8_t volume)
{
    write('v');
    write(volume);
}
void MusicSerial::init(uint16_t baudPrescale)
{
    UCSR1B |= _BV(TXEN1) | _BV(RXEN1) | _BV(RXCIE1);
    UCSR1C |= _BV(UCSZ10) | _BV(UCSZ11);
    UBRR1H = (baudPrescale >> 8);
    UBRR1L = baudPrescale;

    sei();
}

uint8_t MusicSerial::getStatus()
{
    return MusicSerial::stat;
}

ISR(USART1_RX_vect)
{
    uint8_t rcvdByte = UDR1;
    switch(rcvdByte)
    {
    case 'X':
        MusicSerial::stat = MusicSerial::FINISHED;
        break;
    case 'x':
        MusicSerial::stat = MusicSerial::CANCELED;
        break;
    case 'E':
        MusicSerial::stat = MusicSerial::ERROR;
        break;
    default:
        MusicSerial::stat = MusicSerial::UNSET;
    }
}
