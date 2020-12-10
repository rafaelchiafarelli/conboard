
#ifndef DRV_H
#define DRV_H
#include "std_translation_table.hpp"
#include <iostream>
#include "pthread.h"
#include "string.h"
#include "std_translation_table.hpp"
#include "actions.h"

using namespace std;


class externalOUT{
private:
    string outputPath;
public:
    externalOUT(string out){outputPath=out;};
};

class input{
private:
    string inputPath;
public:
    input(string in){inputPath = in;};
};

class drv
{
    private:
        pthread_mutex_t local_mu;
        
        string confPath;
        input in;
        externalOUT outKeyboard;
        externalOUT outMouse;
        

    public:

    virtual void run()=0;

    void outFunction(devActions d);    

    drv(std::string intputPath, std::string keyPath, std::string mousePath):in(intputPath), outKeyboard(keyPath), outMouse(mousePath){

    };

    ~drv(){};
};


#endif