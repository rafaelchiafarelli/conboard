#include "devthread.hpp"
#include <chrono>
#include <thread>
/**
 * Send the command to the output selected
 */ 
void drv::outFunction(devActions d)
{
    switch(d.tp)
    {
        default:
        break;
        case mouse:
        break;
        case keyboard:
        break;
    }
    if(d.delay != 0)
        std::this_thread::sleep_for(std::chrono::microseconds(d.delay));
}