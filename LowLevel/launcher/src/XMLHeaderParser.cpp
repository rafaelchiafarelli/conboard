
#include <iostream>
#include "XMLHeaderParser.h"
#include "keyParser.hpp"

XMLHeaderParser::XMLHeaderParser(const char *_devInfo, const char *_FileName) 
{
	if (createFile(_devInfo, _FileName)) 
	{
		std::cout<<"file created"<<std::endl;
	}
}

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
					ParseOK = ProcessHeader(H);
				}
			}
		}
	}
}

XMLHeaderParser::~XMLHeaderParser()
{
	
if(argv)
for(;counter>0;counter--)
	delete argv[counter];

}

bool XMLHeaderParser::ProcessHeader(rapidxml::xml_node<> *Header)
{
	bool ret = false;
	if (Header)
	{
		rapidxml::xml_node<>* identifier = Header->first_node("identifier", 10, true);
		if(identifier)
		{
			for (rapidxml::xml_node<>* tag_nodes = identifier->first_node("tag", 3, true)
				; tag_nodes
				; tag_nodes = Header->next_sibling("tag", 3, true))
			{
				KeyValue kv;
				kv.value = std::string(tag_nodes->value(),tag_nodes->value_size());
				rapidxml::xml_attribute<> *name_att = tag_nodes->first_attribute("name",4,true);
				if(name_att)
					kv.key = std::string(name_att->value(),name_att->value_size());
				else
				{
					kv.key = "";
				}
				keys.push_back(kv);
			}
			rapidxml::xml_node<>* executable = identifier->first_node("executable", 10, true);
			if(executable)
			{
				rapidxml::xml_attribute<> *exec_name = executable->first_attribute("name",4,true);
				rapidxml::xml_attribute<> *params = executable->first_attribute("params",6,true);
				if(exec_name && params)
				{
					int param_amount = atoi(std::string(params->value(),params->value_size()).c_str());
					
					argv = new char*[param_amount];	
					ExecLine = new char[exec_name->value_size()];				
					strncpy(ExecLine,exec_name->value(),exec_name->value_size());
					counter = 0;
					
					for(rapidxml::xml_node<>* param = executable->first_node("param", 5, true);
						param;
						param = param->next_sibling("param",5,true))
					{
						rapidxml::xml_attribute<> *cmd_name  = param->first_attribute("name",4,true);
						if(cmd_name)
						{
							std::string name(cmd_name->value(),cmd_name->value_size());
							std::string cmd_value(param->value(),param->value_size());
							argv[counter] = new char[cmd_name->value_size()+param->value_size()+5];
							memset(argv[counter],0,cmd_name->value_size()+param->value_size()+5);
							size_t value_s = cmd_name->value_size();
							char *dest = strncpy(argv[counter],cmd_name->value(),value_s) + value_s;
							*dest = ' ';
							dest++;
							char *dest1 = strncpy(dest,param->value(),param->value_size());
							counter++;
							if(counter>param_amount)
								break;
						}
					}
					argv[counter] = new char[1];
					argv[counter] = NULL;
					ret = true;
				}
			}
		}	
	}
	return ret;
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


bool XMLHeaderParser::createFile(const char *devInfo, const char *destName) {
	bool ret = false;

	keyParser data(devInfo,'=');
	std::vector<KeyValue> ParsedData = data.GetParsed();


	// Write xml file ================================

	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "ISO-8859-1"));

	doc.append_node(decl);
	
	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "DEVICE");
	root->append_attribute(doc.allocate_attribute("name", ""));
	root->append_attribute(doc.allocate_attribute("input", ""));
	root->append_attribute(doc.allocate_attribute("output", ""));
	root->append_attribute(doc.allocate_attribute("type", ""));
	doc.append_node(root);
	
	rapidxml::xml_node<>* header = doc.allocate_node(rapidxml::node_element, "header");
	header->append_attribute(doc.allocate_attribute("test","someCooltest"));
	root->append_node(header);

	rapidxml::xml_node<>* identifier = doc.allocate_node(rapidxml::node_element, "identifier");

	for(std::vector<KeyValue>::iterator kv_it = ParsedData.begin();
		kv_it!=ParsedData.end();
		kv_it++)
		{
			if(	(kv_it->key.find("ID_") == 0) ||
				(!kv_it->key.compare("PRODUCT")) ||
				(!kv_it->key.compare("SHLVL")) ||
				(!kv_it->key.compare("DRIVER")) ||
				(!kv_it->key.compare("DEVTYPE")) ||
				(!kv_it->key.compare("DEVNUM")) ||
				(!kv_it->key.compare("MINOR")) ||
				(!kv_it->key.compare("TYPE")) ||
				(!kv_it->key.compare("SEQNUM")) ||
				(!kv_it->key.compare("MAJOR")))
			{
				rapidxml::xml_node<>* tag = doc.allocate_node(rapidxml::node_element, "tag");
				tag->append_attribute(doc.allocate_attribute("name",kv_it->key.c_str()));
				tag->value(kv_it->value.c_str(),kv_it->value.length());
				identifier->append_node(tag);
			}
			else
			{
				rapidxml::xml_node<>* generic = doc.allocate_node(rapidxml::node_element, "generic");
				generic->append_attribute(doc.allocate_attribute("name",kv_it->key.c_str()));
				generic->value(kv_it->value.c_str(),kv_it->value.length());
				identifier->append_node(generic);				
			}
		}

	root->append_node(identifier);
	// Save to file
	std::ofstream file_stored(destName);
	file_stored << doc;
	file_stored.close();
	doc.clear();
	
	return ret;
}



