#pragma once
#include <iostream>
#include <atomic>
#include <vector>
#include <string>
using namespace std;


class raw_midi{
    public:
    string port;
    string devName;
    string sub_name;
    string name;
    int sub;
    int card;
    int device;

};


atomic_bool stop;
vector<raw_midi> hw_ports;

