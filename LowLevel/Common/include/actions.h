#ifndef ACTIONS_H
#define ACTIONS_H
#include "string"
#include <iostream>
#include <sstream>

#include <ostream>
#include <vector>
#include <set>
#include <cstdint>
#include "keyNumber.hpp"
#include "evTypes.hpp"   // evmatch::evTrigger, for evdev input rules
#include <chrono>
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

typedef enum
{
    midi_normal = 0,
    midi_trigger_higher = 1,
    midi_trigger_lower = 2,
    midi_spot = 3,
    midi_blink = 4,
    midi_sysex = 5,
    midi_nomode = 6

}midi_action_mode;

// Lowercase hex, no separators, e.g. "f04312...f7" -- see docs/next-sessions/
// 08-midi-sysex.md for why this shape (survives the ZMQ envelope's ";"-split
// + space-strip framing untouched, since MIDI data bytes 0-127 can otherwise
// collide with those delimiter characters).
inline std::string hexEncode(const std::vector<uint8_t> &bytes) {
    static const char *digits = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (uint8_t b : bytes) {
        out.push_back(digits[(b >> 4) & 0xF]);
        out.push_back(digits[b & 0xF]);
    }
    return out;
}

// Inverse of hexEncode(). Case-insensitive; an odd-length or non-hex input
// yields an empty vector rather than reading past the string (defensive
// against a garbage "sysex" field in a hand-edited board JSON).
inline std::vector<uint8_t> hexDecode(const std::string &hex) {
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::vector<uint8_t> out;
    if (hex.size() % 2 != 0) return out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = nibble(hex[i]);
        int lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back((uint8_t)((hi << 4) | lo));
    }
    return out;
}

class midiActions{
    public:
    midi_action_mode midi_mode = midi_normal;
    midiSignal midi;
    std::vector<uint8_t> sysex;   // only meaningful when midi_mode == midi_sysex
    unsigned long int delay = 0;
    std::string str(){
        char c_str[14];
        std::sprintf(c_str,"%d %d %d",midi.byte[0],midi.byte[1],midi.byte[2]);
        return std::string(c_str);
    };
    std::string ar_str(){
        if (midi_mode == midi_sysex) {
            return "SX:" + hexEncode(sysex);
        }
        char c_str[14];
        std::sprintf(c_str,"[%d,%d,%d]",midi.byte[0],midi.byte[1],midi.byte[2]);
        return std::string(c_str);
    };
    midiActions(){};
    ~midiActions(){};
    friend std::ostream& operator<<(std::ostream &os, const midiActions &dt){
        os<<std::hex<<(unsigned int)dt.midi.byte[0]<<" "<<std::hex<<(unsigned int)dt.midi.byte[1]<<" "<<std::hex<<(unsigned int)dt.midi.byte[2]<<std::endl;
        return os;
    };
    friend std::stringstream& operator<<(std::stringstream &os, const midiActions &dt){
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
        int spot = -1;
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
        int spot=-1;

        devType tp; /* type of the output, either keyboard or mouse ou a midi response*/

        //keyboard
        keyboardActions kData; /* data to be sent to the output*/

        joystickActions joy;
        //mouse
        mouseActions mouse;

        //midi
        midiActions mAct;

        //evdev input trigger (joystick/keyboard/mouse as INPUT); the evdev
        //counterpart of mAct. Populated for input rules; unused for outputs.
        evmatch::evTrigger evtrig;

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
    bool change_mode = false;
    unsigned int change_to = -1;
    devActions in;
    std::vector<devActions> out;
    void clear(){
        out.clear();
        in = devActions();
    }
    Actions(){};
    ~Actions(){};
    bool operator > (const Actions &rhs) const {return in.mAct.midi.asInt>rhs.in.mAct.midi.asInt;}
};

#endif
