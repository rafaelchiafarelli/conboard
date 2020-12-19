#ifndef ACTIONS_H
#define ACTIONS_H
#include "string"
#include <vector>
#include <set>

typedef enum{
    midi,
    keyboard,
    mouse,
    joystick
}devType;


typedef union {
    char byte[4];
    uint32_t asInt;
}midiSignal;


class devActions{
    private: 
        unsigned int index;
    public:
        devType tp; /* type of the output, either keyboard or mouse ou a midi response*/

        //keyboard
        std::string data; /* data to be sent to the output*/

        //mouse
        long dx = 0;
        long gotox = 0;
        long dy = 0;
        long gotoy = 0;
        unsigned int wheel_move = 0;
        bool click = 0;
        bool right_click = 0;

        //midi
        midiSignal midi;

        //delay
        unsigned int delay=0; /* delay in microsseconds to wait after data was sent */
        
        
        devActions(unsigned char b0, unsigned char b1, unsigned char b2){
            index = ((unsigned int)b2)<<16 + ((unsigned int)b1)<<8 + b0;
            tp = devType::midi;
        };
        devActions(){
            index = 0;
            tp = devType::midi;
            }
        unsigned int GetIndex(){return index;}
        void SetIndex(int idx){index = idx;}
        

};



class Actions{

public:
    devActions in;
    std::vector<devActions> out;
    Actions(){};
    ~Actions(){};
    bool operator > (const Actions &rhs) const {return in.midi.asInt>rhs.in.midi.asInt;}
};

#endif
