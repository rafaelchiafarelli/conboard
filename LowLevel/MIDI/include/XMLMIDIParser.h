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

class ModeType{
public:
	std::set<Actions,std::greater<Actions>> body_actions;
	unsigned int index;
	ModeType(std::set<Actions,std::greater<Actions>> b, unsigned int idx){
		body_actions = b;
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
		
		std::set<ModeType,std::greater<ModeType>> *modes;
		rapidxml::xml_document<> xmlDoc;
		devType type;
		unsigned int timeout;
		devActions parseIO(rapidxml::xml_node<> *nodes);
		bool Initializer();
		
		bool loadFile(const std::string &filename);

		void ProcessHeader(rapidxml::xml_node<> *Device);
		void ProcessMainBody(rapidxml::xml_node<> *Device);
	public:
		void Clear(){
			raw_xml.clear();
			xmlDoc.clear();
			loaded = false;
		}
		void Reload();
		devType GetType(){return type;};
		unsigned int GetTimeOut(){return timeout;};
		XMLMIDIParser(std::string FileName,std::set<ModeType,std::greater<ModeType>>  *Mode,std::vector<Actions> *h);
		~XMLMIDIParser();
};

