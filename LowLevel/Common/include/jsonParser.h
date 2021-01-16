#pragma once

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

#include "keyParser.hpp"

#include "actions.h"

class ModeType{
public:
	std::vector<Actions> header;
	std::vector<Actions> body_actions;
	unsigned int index;
	bool is_active;
	ModeType(std::vector<Actions> b, std::vector<Actions> h, unsigned int idx){
		body_actions = b;
		header = h;
		index = idx;
	}
	ModeType(){}
	bool operator > ( const ModeType &rhs) const {return index>rhs.index;}
};

class Executable{
	public:
		std::string exec;
		int param_count;
		std::vector<KeyValue> params;
};


class jsonParser
{
	private:
		bool loaded;
		
		std::vector<KeyValue> Tags;
		std::vector<KeyValue> Generics;
		

		std::vector<Actions> *header_actions;
		std::vector<ModeType> *op_modes;
		
		//loading and parsing
		std::stringstream data;
		rapidjson::Document Doc;

		devType type;
		unsigned int timeout;
		devActions parseIO(rapidjson::Value& act);
		bool Initializer();
		
		bool loadFile(const char *filename);

		void ProcessHeader(rapidjson::Value &header);
		void ProcessMainBody(rapidjson::Value &body);
		devType GetDevType(std::string dType);
	public:
		Executable Ex;
		std::string FileName;
		std::string DevName;
		std::string DevInput;
		std::string DevOutput;
		void Clear(){
			loaded = false;
		}
		void Reload();
		
		devType GetType(){return type;};
		unsigned int GetTimeOut(){return timeout;};
		bool GetLoaded(){return loaded;};
		std::vector<KeyValue> GetTags(){return Tags;};
		jsonParser(std::string filename, std::vector<ModeType> *Mode,std::vector<Actions> *h);
		~jsonParser();
};

