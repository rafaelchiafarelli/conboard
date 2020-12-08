#ifndef KEYTHREAD_HPP
#define KEYTHREAD_HPP
#include "devthread.hpp"
#include "std_translation_table.hpp"
#include "pthread.h"
#include "stdio.h"
#include "fstream"
#include "string.h"
using namespace std;

class keys : public drv {
    public:
    int  a;
    void do_stuff();
    void run(){};
    void translate();
    
    keys(string inputPath, string outputPath, string confPath):drv(inputPath,outputPath,confPath){
        a = 0;
        cout<<"this is a message to you"<<endl;

    };
    ~keys(){};

};


#endif