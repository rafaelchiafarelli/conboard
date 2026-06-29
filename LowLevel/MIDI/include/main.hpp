#pragma once
#include <iostream>
#include <atomic>
#include <vector>
#include <string>
#include "midithread.hpp"
using namespace std;




// The process-wide stop flag now lives in condev::runDevice (runDevice.hpp).
vector<raw_midi> hw_ports;

