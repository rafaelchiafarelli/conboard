#include "keyParser.hpp"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <string>


keyParser::keyParser(const char *fileName, char token)
{
    std::ifstream infile(fileName);
    std::string a, b; char c = token; 
    std::string line;
    while (std::getline(infile,line)) 
    {
        size_t delimiter_pos = line.find(token);
        if(delimiter_pos != std::string::npos)
        {
            a = line.substr(0,delimiter_pos);
            b = line.substr(delimiter_pos+1,std::string::npos);
            KeyValue kv = {a,b};
            parsed.push_back(kv);
        }
    }
}

