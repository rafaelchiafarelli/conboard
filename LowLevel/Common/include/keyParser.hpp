#pragma once

#include <map>
#include <stdio.h>
#include <iostream>
#include <vector>

class KeyValue{
    public:
        std::string key = "";
        std::string value = "";
};

class keyParser
{
    std::vector<KeyValue> parsed;

    public:
        keyParser(const char *fileName, char token);
        std::vector<KeyValue> GetParsed(){return parsed;};

};