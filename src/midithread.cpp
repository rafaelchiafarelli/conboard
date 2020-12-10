#include "midithread.hpp"


void MIDI::parse()
{

}


/**
 * Function to read the input, filter the events and generate outputs as defined in XML 
 * 
 */ 
void MIDI::run(){

}

MIDI::MIDI(string actionPath, 
            string inputPath, 
            string outputPath, 
            string confPath):drv(inputPath,outputPath,confPath), 
                             xml(confPath,
                                &modes,
                                &header)
{
    count = 0;
}

void MIDI::execHeader(){
    for(std::vector<Actions>::iterator it = header.begin();
        it != header.end();
        it++)
    {
        for(std::vector<devActions>::iterator devIt = it->out.begin();
            devIt != it->out.end();
            devIt++)
        {
            drv::outFunction(*devIt);
        }
    }
}
