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

using namespace std;


class MIDI : public drv {

    private:
        int count;
        vector<Actions> ActionsToBeTaken;
     
    public:
    int  a;
    void parse();
    
    void run();
    
    
    MIDI(string config, string inputPath, string outputPath, string confPath);
    ~MIDI(){};

};


#endif