
#include <iostream>
#include "XMLHeaderParser.h"
#include "keyParser.hpp"


XMLHeaderParser::XMLHeaderParser(const char *_FileName) 
{
	if (loadFile(_FileName)) 
	{

		{
			rapidxml::xml_node<> *D = xmlDoc.first_node("DEVICE", 6, true);
			if (D)
			{
				rapidxml::xml_node<> *H = D->first_node("header",6,true);
				if(H)
				{
					ProcessHeader(H);
				}
			}
		}
	}
}

XMLHeaderParser::~XMLHeaderParser()
{

}

void XMLHeaderParser::ProcessHeader(rapidxml::xml_node<> *Header)
{
	if (Header)
	{
		rapidxml::xml_node<>* identifier = Header->first_node("identifier", 10, true);
		if(identifier)
		{
			for (rapidxml::xml_node<>* tag_nodes = identifier->first_node("tag", 3, true)
				; tag_nodes
				; tag_nodes = Header->next_sibling("tag", 3, true))
			{
				
			}
		}	
	}
}

bool XMLHeaderParser::loadFile(const char *filename) {
	bool ret = false;
	
	rapidxml::file<> xmlFile(filename); // Default template is char
	
	if(xmlFile.size()>0)
		ret = true;
	

	enum {
		PARSE_FLAGS = rapidxml::parse_non_destructive
	};
	
	xmlDoc.parse<PARSE_FLAGS>(xmlFile.data());
    
	
	return ret;
}


bool XMLHeaderParser::createFile(const char *sourceName, const char *destName) {
	bool ret = false;

	keyParser data(sourceName,'=');
	std::vector<KeyValue> ParsedData = data.GetParsed();
	// Write xml file ================================

	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "ISO-8859-1"));
	doc.append_node(decl);
	
	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "DEVICE");
	root->append_attribute(doc.allocate_attribute("timeout", "0"));

	root->append_attribute(doc.allocate_attribute("type", "example"));
	doc.append_node(root);
	
	rapidxml::xml_node<>* child = doc.allocate_node(rapidxml::node_element, "childnode");
	root->append_node(child);
	
	// Save to file
	std::ofstream file_stored("file_stored.xml");
	file_stored << doc;
	file_stored.close();
	doc.clear();
	
	return ret;
}



