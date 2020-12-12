/**
 * A device is a set of actions executed during boot-up and a set of actions 
 * that are taken in a by event manner.
 * Device has a personalized thread to be executed continously reading the events
 * from the phisycal device.
 * As the device sends input signals, these becomes actions and responses could
 * could be output to the keyboard or mouse, or midi responses to the sender.
 * 
 * 
 */ 

#ifndef KEYTHREAD_HPP
#define KEYTHREAD_HPP

#include "std_translation_table.hpp"
#include "pthread.h"
#include "stdio.h"
#include "fstream"
#include "string.h"
#include <vector>
#include "actions.h"
#include "XMLMIDIParser.h"
#include <alsa/asoundlib.h>

#include <set>
#include <atomic>


using namespace std;
#define PORT_NAME_SIZE 10;

class MIDI{

    private:
        
        std::vector<Actions> header;
        std::set<ModeType,std::greater<ModeType>> modes;
        XMLMIDIParser xml;
        snd_rawmidi_t *input;
        snd_rawmidi_t *output;
        atomic_bool stop;
        char port_name[PORT_NAME_SIZE];
        int timeout;
        void execHeader();
        void sig_handler(int dummy);
        void parse();

    public:
        void thread_func();
        MIDI(char *port_name);
        ~MIDI(){};

};


#endif