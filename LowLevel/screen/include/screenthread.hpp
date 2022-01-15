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

#ifndef TouchScreenTHREAD_HPP
#define TouchScreenTHREAD_HPP

#include "stdio.h"
#include "actions.h"
#include "jsonParser.h"
#include <sstream>
#include <string>
#include <alsa/asoundlib.h>

#include <thread>
#include <set>
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <oActions.hpp>

#include "zmq_coms.hpp"

using namespace std;
#define PORT_NAME_SIZE 10
#define MILLISECONDS_TIMEOUT 10


class TouchScreen : private oActions {

    private:

        std::string jsonFileName;

        mutex locking_mechanism;
        std::thread *in_thread;
        std::thread *out_thread;
        std::thread *thcoms;
        zmq_coms *com;
        
        
        std::queue<std::vector<devActions> > oQueue;
        void in_func(); //midi input handler
        void out_func(); //keyboard and mouse handler
        bool outToFile;
        std::string outFileName;
        ofstream outFileStream;

        atomic_bool send;
        atomic_bool stop;
        int timeout;

        std::vector<Actions> header;
        std::vector<ModeType> modes;
        ModeType CurrentMode;
        jsonParser json;

        unsigned int SelectedMode;


        void saveJSON();
        void changeMode(std::vector<Actions>::iterator it_act);
        void execHeader();
        void parse();
        void processInput(midiSignal midiS);
        void processMode(ModeType m);
        void send_midi(char *send_data, size_t send_data_length);
        void send_mouse(mouseActions mouse);

        void startup();

        std::vector<std::string> explode(std::string const & s, char delim);
    public:
        void coms_handler();
        void Stop();
        void Reload();
        void outStop();
        bool outFile(string name);


        TouchScreen( string xmlFileName, vector<raw_midi> hw_ports);
        ~TouchScreen();

};


#endif