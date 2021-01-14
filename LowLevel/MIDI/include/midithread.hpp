/**
 * A device is a set of actions executed during boot-up and a set of actions 
 * that are taken in a by event manner.
 * Device has a personalized thread to be executed continously reading the events
 * from the phisycal device.
 * As the device sends input signals, these becomes actions and responses could
 * could be output to the keyboard or mouse, or midi responses to the sender.
 * 
 * features:
 *  - change mode: When a specific action happens (as defined in the xml). We must change
 *  the current mode (all the actions are descripted by another mode). More than that, 
 *  mode can be changed by increase, decrease or specific.
 *  - delay: a time waited in milliseconds, as described in the xml, for the next output to be executed
 *  - action queue: since delay stop the output of this thread, a queue must be exacted.
 *  - blink: an outputn special type. It will send the midi on and than send the midi off
 */ 

#ifndef KEYTHREAD_HPP
#define KEYTHREAD_HPP

#include "stdio.h"
#include "actions.h"
#include "jsonParser.h"

#include <alsa/asoundlib.h>
#include <thread>
#include <set>
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <oActions.hpp>

using namespace std;
#define PORT_NAME_SIZE 10
#define MILLISECONDS_TIMEOUT 10


class MIDI : private oActions{

    private:


        mutex locking_mechanism;
        std::thread *in_thread;
        std::thread *out_thread;
        std::queue<std::vector<devActions> > oQueue;
        void in_func(); //midi input handler
        void out_func(); //keyboard and mouse handler
        bool outToFile;
        std::string outFileName;
        ofstream outFileStream;

        atomic_bool send;
        atomic_bool stop;
        int timeout;
        ModeType l_mode;
        Actions l_action;

        std::vector<Actions> header;
        std::vector<ModeType> modes;
        jsonParser json;

        unsigned int SelectedMode;

        snd_rawmidi_t *input;
        snd_rawmidi_t *output;
        char port_name[PORT_NAME_SIZE];
        
        void execHeader();
        void parse();
        void processInput(midiSignal midiS);
        void send_midi(char *send_data, size_t send_data_length);
        void send_mouse(mouseActions mouse);
        void send_joystick(){};

    public:
    
        void Stop();
        void Reload();
        void outStop();
        bool outFile(string name);
        void oMouse(mouseActions){};
        
        void oJoystick(joystickActions){};
        void oKeyboard(keyboardActions){};
        MIDI(char *port_name, string xmlFileName, char *hid_name);
        ~MIDI();

};


#endif