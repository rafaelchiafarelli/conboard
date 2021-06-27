#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include "stdio.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

#include <set>
#include <vector>
#include <iostream>
#include "list"
#include <cstddef>
#include <cassert>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

class config{
    public:
		//loading and parsing
		std::string data;
		rapidjson::Document Doc;
        config(std::string fileName){};
        ~config(){};
};

#endif