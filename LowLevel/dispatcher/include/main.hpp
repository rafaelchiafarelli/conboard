#pragma once
#include <iostream>
#include <atomic>
#include <vector>
#include <string>
#include "midithread.hpp"
using namespace std;


atomic_bool stop;
vector<raw_midi> hw_ports;

