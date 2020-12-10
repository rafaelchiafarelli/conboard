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
#include "devthread.hpp"
#include "std_translation_table.hpp"
#include "pthread.h"
#include "stdio.h"
#include "fstream"
#include "string.h"
#include <vector>
#include "actions.h"
#include "XMLMIDIParser.h"
using namespace std;


class MIDI : public drv {

    private:
        int count;
        std::vector<Actions> header;
        std::vector<ModeType> modes;
        XMLMIDIParser xml;
        void execHeader();
        int  a;
        void parse();
        void run();
    public:
        MIDI(string config, string inputPath, string outputPath, string confPath);
        ~MIDI(){};

};


#endif