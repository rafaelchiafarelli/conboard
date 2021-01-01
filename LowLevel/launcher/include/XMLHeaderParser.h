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

#include "rapidxml_utils.hpp"
#include <set>
#include "rapidxml_ext.hpp"
#include "rapidxml.hpp"
#include <fstream>
#include <sstream>
#include "actions.h"

class Header{
public:


};

class XMLHeaderParser
{
	private:
		bool loaded;
		std::string FileName;
		std::string DevName;
		std::string DevInput;
		std::string DevOutput;

		devType dType;

		rapidxml::xml_document<> xmlDoc;
		
		
		void ProcessHeader(rapidxml::xml_node<> *Header);

	public:
		devType GetType(){return dType;};
		bool loadFile(const char *filename);
		bool createFile(const char *sourceName, const char *destName);
		XMLHeaderParser(const char *filename);
		~XMLHeaderParser();
};

