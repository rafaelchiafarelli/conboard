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

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


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

class JsonMIDIParser
{
	private:
		bool loaded;
		std::string FileName;
		std::string DevName;
		std::string DevInput;
		std::string DevOutput;
		
		std::vector<Actions> *header_actions;
		std::vector<ModeType> *modes;
		
		//loading and parsing
		std::string filename;
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
		void Clear(){
		
			loaded = false;
		}
		void Reload();
		devType GetType(){return type;};
		unsigned int GetTimeOut(){return timeout;};
		JsonMIDIParser(const char *filename,std::vector<ModeType> *Mode,std::vector<Actions> *h);
		~JsonMIDIParser();
};

