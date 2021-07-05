#ifndef DISPATCHER_H
#define DISPATCHER_H
#include <iostream>
#include <atomic>
#include <thread>

#include "configParser.hpp"

class dispatcher{
    private:
        config disp;
        std::string fileName;
        std::thread *zmq_io;
        std::thread *zmq_coms;
        std::thread *http;
        std::thread *screen;
        std::thread *vault;
        std::thread *shared;
        
    public:
        dispatcher();
        dispatcher(std::string fileName, std::atomic_bool *stop);
        ~dispatcher();
};

#endif 