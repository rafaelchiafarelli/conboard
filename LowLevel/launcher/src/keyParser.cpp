#include "keyParser.hpp"
#include <iostream>
#include <fstream>
#include <stdlib.h>


keyParser::keyParser(const char *fileName, char token)
{
    std::ifstream infile(fileName);
    std::string a, b; char c; 
    while ((infile >> a >> c >> b) && (c == token)) 
    {
        KeyValue kv = {a,b};
        parsed.push_back(kv);
    }
}

