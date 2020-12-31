#ifndef ACTIONS_H
#define ACTIONS_H
#include "string"
#include <iostream>
#include <ostream>
#include <vector>
#include <set>
#include "keyNumber.hpp"

/**
 * 
 */ 
typedef enum{
    oneKey,
    hotkey,
    text
}keyType;

/**
 * 
 */
typedef enum{
    hold,
    not_hold,
    hold_delay
}holdType;

/**
 * 
 */
typedef enum{
    notype,
    midi,
    keyboard,
    mouse,
    joystick
}devType;

/**
 * 
 * 
 */
typedef union {
    char byte[4];
    uint32_t asInt;
}midiSignal;


class midiActions{
    public:
    midiSignal midi;
    unsigned long int delay = 0;
    midiActions(){};
    ~midiActions(){};
    friend std::ostream& operator<<(std::ostream &os, const midiActions &dt){
        os<<std::hex<<(unsigned int)dt.midi.byte[0]<<" "<<std::hex<<(unsigned int)dt.midi.byte[1]<<" "<<std::hex<<(unsigned int)dt.midi.byte[2]<<std::endl;
        return os;
    };

};

/**
 * 
 * 
 */ 
class joystickActions{
    private:
    public:
    joystickActions(){};
    ~joystickActions(){};
    friend std::ostream& operator<<(std::ostream &os, const joystickActions &dt){
        os<<"no joystickActions yet";
        return os;
    };
};

/**
 * 
 * 
 */ 
class keyboardActions{
    public:
        keyType type = oneKey;
        std::string data = "";
        holdType hold = not_hold;
        unsigned int delay = 0;
        keyboardActions(){};
        ~keyboardActions(){};
        friend std::ostream& operator<<(std::ostream &os, const keyboardActions &dt){
            os<<"type:"<<dt.type<<" data:"<<dt.data<<" delay:"<<dt.delay;
            return os;
        };
};
/**
 * 
 * 
 * 
 */ 
class mouseActions{
    public:
        long dx = 0;
        long gotox = 0;
        long dy = 0;
        long gotoy = 0;
        unsigned int wheel_move = 0;
        bool click = 0;
        bool right_click = 0;
        unsigned long int delay = 0;
        mouseActions(){};
        ~mouseActions(){};
    friend std::ostream& operator<<(std::ostream &os, const mouseActions &dt){
        os<<"dx:"<<dt.dx<<" dy:"<<dt.dy<<" gotox:"<<dt.gotox<<" gotoy:"<<dt.gotoy<<" whm:"<<dt.wheel_move<<" click:"<<dt.click<<" rclick"<<dt.right_click;
        return os;
    };
};
/**
 * 
 * 
 */ 
class devActions{
    private: 
        unsigned int index;
    public:
        devType tp; /* type of the output, either keyboard or mouse ou a midi response*/

        //keyboard
        keyboardActions kData; /* data to be sent to the output*/

        joystickActions joy;
        //mouse
        mouseActions mouse;

        //midi
        midiActions mAct;


        devActions(){
            index = 0;
            tp = devType::notype;
        };
        devActions(unsigned char b0, unsigned char b1, unsigned char b2){
            index = ((unsigned int)b2)<<16 + ((unsigned int)b1)<<8 + b0;
            mAct.midi.asInt = index;
            tp = devType::midi;
        };

        devActions(std::string kD, keyType ktp){
            kData.data = kD;
            kData.type= ktp;
            tp = devType::keyboard;
        };
        devActions( unsigned long int dx,
                    unsigned long int gotox,
                    unsigned long int dy,
                    unsigned long int gotoy,
                    unsigned int wheel_move,
                    bool click,
                    bool right_click){
            mouse.dx = dx;
            mouse.gotox = gotox,
            mouse.dy = dy;
            mouse.gotoy = gotoy;
            mouse.wheel_move = wheel_move;
            mouse.click = click;
            mouse.right_click = right_click;
            tp = devType::mouse;
        };
        unsigned int GetIndex(){return index;}
        void SetIndex(int idx){index = idx;}

        friend std::ostream& operator<<(std::ostream &os, const devActions &devAct){
            switch(devAct.tp)
            {
                case devType::keyboard:
                os<<"keyboard: "<<devAct.kData;
                break;
                case devType::joystick:
                os<<"joystick: "<<devAct.joy;
                break;
                case devType::midi:
                os<<"midi: "<<devAct.mAct;
                break;
                case devType::mouse:
                os<<"mouse: "<<devAct.mouse;
                break;
            }
        return os;
        };
};
/**
 * 
 * 
 */ 
class Actions{

public:
    devActions in;
    std::vector<devActions> out;
    Actions(){};
    ~Actions(){};
    bool operator > (const Actions &rhs) const {return in.mAct.midi.asInt>rhs.in.mAct.midi.asInt;}
};

#endif
