#include <iostream>
#include <atomic>
#include <csignal>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <vector>
#include "midithread.hpp"
#include "keythread.hpp"
#include "devthread.hpp"
#include "XMLMIDIParser.h"
#include "std_translation_table.hpp"
using namespace std;

class MainCtx{
    public:
        atomic_bool dead;
};

static MainCtx ctx;


void signalHandler( int signum ) {
    cout << "Interrupt signal (" << signum << ") received.\n";
    ctx.dead = true;
}


/**
 * 
    //main mgr -- pool de resources from /dev/input and fires the threads

    //listen to SIGTERM and other SIGNALS in orther to kill the threads with no devices attached

    //keep listening for newly added devices

    //clean everything up

    //return the code
 */ 

int main(int argc, char **argv)
{
    int ret = 0;
    MIDI One(string("/home/pi/MIDI_DJTech4Mix.xml"), 
            string("/home/pi/MIDI_DJTech4Mix.xml"), 
            string("/home/pi/MIDI_DJTech4Mix.xml"),
            string("/home/pi/MIDI_DJTech4Mix.xml"));

    ctx.dead = false;
    signal(SIGINT, signalHandler);  

    char eventPathStart[] = "/dev/input/event";
    std::vector<pollfd> poll_fds;
    for (auto &i : filesystem::directory_iterator("/dev/input")) {
        if(i.is_character_file()) {
            std::string view(i.path());
            int evfile = open(view.c_str(), O_RDONLY | O_NONBLOCK);
            std::cout << "Adding " << i.path() << std::endl;
            poll_fds.push_back({evfile, POLLIN, 0});
        }
    }

























































    return ret;
}