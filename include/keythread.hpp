#ifndef KEYTHREAD_HPP
#define KEYTHREAD_HPP

#include "std_translation_table.hpp"
#include "pthread.h"
#include "stdio.h"
#include "iostream"
#include "string.h"
using namespace std;

class keys {
    public:
    int  a;
    void do_stuff();
    void run(){};
    void translate();
    
    keys(string inputPath, string outputPath, string confPath){
        a = 0;
        cout<<"this is a message to you"<<endl;

    };
    ~keys(){};

};


#endif