#pragma once

#include "stdio.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>


#include "list"
#include <cstddef>
#include <cassert>
#define RAPIDXML_NO_STDLIB
//#define RAPIDXML_NO_EXCEPTIONS
#include "rapidxml.hpp"
#include "actions.h"
#include <set>
#include <vector>
class ModeType{
public:
	std::vector<Actions> header;
	std::vector<Actions> body_actions;
	unsigned int index;
	ModeType(std::vector<Actions> b, std::vector<Actions> h, unsigned int idx){
		body_actions = b;
		header = h;
		index = idx;
	}
	ModeType(){}
	bool operator > ( const ModeType &rhs) const {return index>rhs.index;}
};

class XMLMIDIParser
{
	private:
		bool loaded;
		std::string FileName;
		std::string DevName;
		std::string DevInput;
		std::string DevOutput;
		
		std::vector<Actions> *header_actions;
		std::vector<ModeType> *modes;

		rapidxml::xml_document<> xmlDoc;
		devType type;
		unsigned int timeout;
		devActions parseIO(rapidxml::xml_node<> *nodes);
		bool Initializer();
		
		bool loadFile(const char *filename);

		void ProcessHeader(rapidxml::xml_node<> *Device);
		void ProcessMainBody(rapidxml::xml_node<> *Device);
	public:
		void Clear(){
			xmlDoc.clear();
			loaded = false;
		}
		void Reload();
		devType GetType(){return type;};
		unsigned int GetTimeOut(){return timeout;};
		XMLMIDIParser(const char *filename,std::vector<ModeType> *Mode,std::vector<Actions> *h);
		~XMLMIDIParser();
};

