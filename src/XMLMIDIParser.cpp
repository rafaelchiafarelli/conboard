
#include "XMLMIDIParser.h"


XMLMIDIParser::XMLMIDIParser(std::string FileName) {
	loaded = false;
	if (loadFile(FileName)) 
	{
		if(!raw_xml.empty())
		{
			const auto tmp = raw_xml;
			enum {
				PARSE_FLAGS = rapidxml::parse_non_destructive
			};

			// NOTE : There is a `const_cast<>`, but `rapidxml::parse_non_destructive`
			//        guarantees `data` is not overwritten.
			
			xmlDoc.parse<PARSE_FLAGS>(const_cast<char*>(raw_xml.data()));
			if (xmlDoc.first_node("DEVICE", 6, true))
			{

				loaded = Initializer();
			}
		}
	}
}


XMLMIDIParser::~XMLMIDIParser()
{
		header_actions.clear();
		body_actions.clear();
}

void XMLMIDIParser::ProcessMainBody(rapidxml::xml_node<> *Body)
{
	if (Body)
	{
		for (rapidxml::xml_node<>* action_nodes = Body->first_node("action", 6, true)
			; action_nodes
			; action_nodes = Body->next_sibling("action", 6, true))
		{
			rapidxml::xml_node<>* in_nodes = action_nodes->first_node("input",5,true);
			Actions action;
			if(in_nodes)
			{
				action.in = parseIO(in_nodes);
			}

			for (rapidxml::xml_node<>* out_nodes = action_nodes->first_node("output", 6, true)
				; out_nodes
				; out_nodes = action_nodes->next_sibling("output", 6, true))
			{
				action.out.push_back(parseIO(out_nodes));
			}
			body_actions.push_back(action);
		}
	}
}

devActions XMLMIDIParser::parseIO(rapidxml::xml_node<> *nodes)
{
	devActions ret;
	char *in_type = nodes->first_attribute("type",4,true)->value();
	if(!strncmp(in_type, "mouse",5))
	{
		ret.tp = mouse;
		char *dx = nodes->first_attribute("dx",2,true)->value();
		char *dy = nodes->first_attribute("dy",2,true)->value();
		char *wheel_move = nodes->first_attribute("wheel_move",10,true)->value();
		char *gotox = nodes->first_attribute("gotox",5,true)->value();
		char *gotoy = nodes->first_attribute("gotoy",5,true)->value();
		char *click = nodes->first_attribute("click",5,true)->value();
		char *right_click = nodes->first_attribute("right_click",11,true)->value();
		char *delay = nodes->first_attribute("delay",6,true)->value();

		if(dx)
		{
			ret.dx = atoi(dx);
		}
		if(dy)
		{
			ret.dy = atoi(dy);
		}
		if(wheel_move)
		{
			ret.wheel_move = atoi(wheel_move);
		}
		if(gotox)
		{
			ret.gotox = atoi(gotox);
		}
		if(gotoy)
		{
			ret.gotoy = atoi(gotoy);
		}
		if(click)
		{
			ret.click = atoi(click);
		}
		if(right_click)
		{
			ret.right_click = atoi(right_click);
		}

		if(delay)
		{
			ret.delay = atoi(delay);
		}

	} else if(!strncmp(in_type, "keyboard",8))
	{
		ret.tp = keyboard;
		char *data = nodes->first_attribute("data",4,true)->value();
		ret.data = std::string(data,nodes->first_attribute("data",4,true)->value_size());

	} else if(!strncmp(in_type, "midi",4))
	{
		ret.b0 = 0;
		ret.b1 = 0;
		ret.b2 = 0;
		char *ptr; //do not use
		if (nodes->first_attribute("b0", 2, true))
		{
			ret.b0 = (unsigned char) strtol(nodes->first_attribute("b0", 2, true)->value(),
				&ptr, 16);
		}

		if (nodes->first_attribute("b1", 2, true))
		{
			ret.b1 = (unsigned char)strtol(nodes->first_attribute("b1", 2, true)->value(),
				&ptr, 16);
		}

		if (nodes->first_attribute("b2", 2, true))
		{
			ret.b2 = (unsigned char)strtol(nodes->first_attribute("b2", 2, true)->value(),
				&ptr, 16);
		}
	}
	return ret;
}

void XMLMIDIParser::ProcessHeader(rapidxml::xml_node<> *Header)
{
	if (Header)
	{
		for (rapidxml::xml_node<>* action_nodes = Header->first_node("action", 6, true)
			; action_nodes
			; action_nodes = Header->next_sibling("action", 6, true))
		{
			rapidxml::xml_node<>* in_nodes = action_nodes->first_node("input",5,true);
			Actions action;
			if(in_nodes)
			{
				action.in = parseIO(in_nodes);
			}

			for (rapidxml::xml_node<>* out_nodes = action_nodes->first_node("output", 6, true)
				; out_nodes
				; out_nodes = action_nodes->next_sibling("output", 6, true))
			{
				action.out.push_back(parseIO(out_nodes));
			}
			header_actions.push_back(action);
		}
	}
}

bool XMLMIDIParser::Initializer()
{
	bool ret = false;
	rapidxml::xml_node<> *Device = xmlDoc.first_node("DEVICE", 6, true);
	if(Device)
	{
		if (Device->first_attribute("name", 4, true))
		{
			DevName = std::string(Device->first_attribute("name", 4, true)->value(), 
				Device->first_attribute("name", 4, true)->value_size());
		}
		if (Device->first_attribute("input", 5, true))
		{
			DevInput = std::string(Device->first_attribute("input", 5, true)->value(), 
				Device->first_attribute("input", 5, true)->value_size());
		}
		if (Device->first_attribute("output", 6, true))
		{
			DevOutput = std::string(Device->first_attribute("output", 6, true)->value(), 
				Device->first_attribute("output", 6, true)->value_size());
		}

	rapidxml::xml_node<> *Header = xmlDoc.first_node("header", 6, true);
	if(Header)
		ProcessHeader(Header);

	rapidxml::xml_node<> *Body = xmlDoc.first_node("body", 4, true);
	if(Body)
		ProcessMainBody(Body);
		
	ret = true;
	}
	return ret;
}

bool XMLMIDIParser::loadFile(const std::string &filename) {
	bool ret = false;
	std::ifstream is( filename, std::ifstream::binary | std::ifstream::ate);
	
	if (is) {
		ret = true;
		raw_xml.resize(static_cast<size_t>(is.tellg()));
		is.seekg(0);
		is.read(raw_xml.data(), raw_xml.size());
		raw_xml.push_back(0);
	}
	return ret;
}


