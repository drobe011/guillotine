/*
 * musicserial.h
 *
 * Created: 7/20/2013 7:57:40 PM
 *  Author: dave
 */


#ifndef MUSICSERIAL_H_
#define MUSICSERIAL_H_

#include <avr/io.h>
#include "button.h"
#include <debug/debugserial.h>

#define MAXBUFFSIZE1 16

////SOUND VARIABLES SETUP # IS NUMBER IN SOUND FILE ###xxx.MP3
////TO SKIP PLAYING, COMMENT OUT THE DIRECTIVE
#define SND_STARTUP 1
#define SND_ENTERINIT 2
#define SND_LEAVEINIT 3
#define SND_ENTERERROR 4
#define SND_LEAVEERROR 5
#define SND_ENTERDEBUG 6
#define SND_LEAVEDEBUG 7
#define SND_LOWER_BEFORE 8
#define SND_LOWER_ON 9
#define SND_LOWER_AFTER 10
#define SND_HEAD_LOWER_BEFORE 11
#define SND_HEAD_LOWER_ON 12
#define SND_HEAD_LOWER_AFTER 13
#define SND_BODY_BEFORE 14
#define SND_BODY_ON 15
#define SND_BODY_AFTER 16
#define SND_INT1_BEFORE 17
#define SND_INT1_ON 18
#define SND_INT1_AFTER 19
//#define SND_RAISE_BEFORE 20
//#define SND_RAISE_ON 21
//#define SND_RAISE_AFTER 22
//#define SND_HEAD_RAISE_BEFORE 23
//#define SND_HEAD_RAISE_ON 24
//#define SND_HEAD_RAISE_AFTER 25
//#define SND_HEAD_TILT_BEFORE 26
//#define SND_HEAD_TILT_ON 27
//#define SND_HEAD_TILT_AFTER 28
//#define SND_LOWER_TILT_BEFORE 29
//#define SND_LOWER_TILT_ON 30
//#define SND_LOWER_TILT_AFTER 31
//#define SND_LOWER_HOIST_BEFORE 32
//#define SND_LOWER_HOIST_ON 33
//#define SND_LOWER_HOIST_AFTER 34
//#define SND_INT2_BEFORE 35
//#define SND_INT2_ON 36
//#define SND_INT2_AFTER 37


class MusicSerial
{
    private:
        void init(uint16_t baud);
        DebugSerial *dbS;
    public:
        MusicSerial();
        MusicSerial(double, double = F_CPU);
        void write(uint8_t);
        uint8_t getStatus();
        enum status {UNSET, PLAY, FINISHED, CANCELED, ERROR};
        volatile static status stat;
        void togglePlay();
        void stop();
        void trigger(uint8_t);
        uint8_t playHold(uint8_t, Button*);
        void nextTrack();
        void prevTrack();
        void volume(uint8_t);

    protected:
};

#endif /* MUSICSERIAL_H_ */
