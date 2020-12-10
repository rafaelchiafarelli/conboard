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

class ModeType{
public:
	std::vector<Actions> body_actions;
	unsigned int index;
	ModeType(std::vector<Actions> b, unsigned int idx){
		body_actions = b;
		index = idx;
	}
};

class XMLMIDIParser
{
	private:
		bool loaded;
		std::string DevName;
		std::string DevInput;
		std::string DevOutput;
		std::vector<char> raw_xml;
		std::vector<Actions> *header_actions;
		
		std::vector<ModeType> *modes;
		rapidxml::xml_document<> xmlDoc;
		devType type;
		devActions parseIO(rapidxml::xml_node<> *nodes);
		bool Initializer();
		
		bool loadFile(const std::string &filename);

		void ProcessHeader(rapidxml::xml_node<> *Device);
		void ProcessMainBody(rapidxml::xml_node<> *Device);
	public:
		XMLMIDIParser(std::string FileName,std::vector<ModeType> *Mode,std::vector<Actions> *h);
		~XMLMIDIParser();
};

