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
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"


#include "keyParser.hpp"

#include "actions.h"

class ModeType{
public:
	std::vector<Actions> header;
	std::vector<Actions> body_actions;
	int index = -1;
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
		
		std::string port;
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
		std::string data;
		rapidjson::Document Doc;

		devType type;
		unsigned int timeout;
		devActions parseIO(rapidjson::Value& act);
		bool Initializer();
		
		bool loadFile();

		void ProcessHeader(rapidjson::Value &header);
		bool hasExec=false;
		void ProcessMainBody(rapidjson::Value &body);
		devType GetDevType(std::string dType);

		void Save(std::string _FileName){
			std::ofstream ofs(_FileName);
			rapidjson::OStreamWrapper osw(ofs);
			rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
			Doc.Accept(writer);
		}
	public:
		Executable Ex;
		std::string FileName;
		std::string DevName;
		std::string DevInput;
		std::string DevOutput;
		void Clear(){
			loaded = false;
		}
		void Reload(std::string _FileName, std::vector<ModeType> *Mode,std::vector<Actions> *h);

		void SaveToFile(std::string _FileName){
				Save(_FileName);
			};
		void SaveFile(){
				Save(FileName);
			};
		devType GetType(){return type;};
		unsigned int GetTimeOut(){return timeout;};
		bool GetLoaded(){return loaded;};
		bool GetHasExec(){return hasExec;};
		std::vector<KeyValue> GetTags(){return Tags;};
		jsonParser(std::string filename, std::vector<ModeType> *Mode,std::vector<Actions> *h);
		~jsonParser();
};

