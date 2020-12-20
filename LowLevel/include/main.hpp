#pragma once
#include <iostream>

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