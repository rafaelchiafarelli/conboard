
#include <iostream>
#include "XMLMIDIParser.h"
#include <set>


XMLMIDIParser::XMLMIDIParser(std::string FileName, std::set<ModeType,std::greater<ModeType>> *Mode,std::vector<Actions> *h) {
	header_actions = h;
	modes = Mode;
	loaded = false;
	FileName = FileName;
	std::cout<<"XMLMIDIParser"<<std::endl;
	Reload();
	std::cout<<"XMLMIDIParser END"<<std::endl;
}
void XMLMIDIParser::Reload(){
	std::cout<<"Reload"<<std::endl;
	if(loaded == false)
	{
		if (loadFile(FileName)) 
		{
			std::cout<<"Reload"<<std::endl;
			if(!raw_xml.empty())
			{
				const auto tmp = raw_xml;
				enum {
					PARSE_FLAGS = rapidxml::parse_non_destructive
				};
				xmlDoc.parse<PARSE_FLAGS>(const_cast<char*>(raw_xml.data()));
				if (xmlDoc.first_node("DEVICE", 6, true))
				{
					loaded = Initializer();
				}
			}
		}
	}
}

XMLMIDIParser::~XMLMIDIParser()
{

}

void XMLMIDIParser::ProcessMainBody(rapidxml::xml_node<> *Body)
{
	std::cout<<"ProcessMainBody"<<std::endl;
	if (Body)
	{
		std::cout<<"Body exists"<<std::endl;
		for (rapidxml::xml_node<>* xmlmodes = Body->first_node("mode", 4, true)
			; xmlmodes
			; xmlmodes = xmlmodes->next_sibling("mode", 4, true))
		{		
			std::cout<<"Mode"<<std::endl;
			char *idtag = xmlmodes->first_attribute("id",2,true)->value();
			unsigned int idx;
			if(idtag)
			{
				idx = atoi(idtag);
			}
			std::set<Actions,std::greater<Actions>> body_actions;
			for (rapidxml::xml_node<>* action_nodes = xmlmodes->first_node("action", 6, true)
				; action_nodes
				; action_nodes = action_nodes->next_sibling("action", 6, true))
			{
				rapidxml::xml_node<>* in_nodes = action_nodes->first_node("input",5,true);
				Actions action;
				
				if(in_nodes)
				{
					action.in = parseIO(in_nodes);
				}
				for (rapidxml::xml_node<>* out_nodes = action_nodes->first_node("output", 6, true)
					; out_nodes
					; out_nodes = out_nodes->next_sibling("output", 6, true))
				{
					action.out.push_back(parseIO(out_nodes));
				}
				body_actions.insert(action);
			}
			std::cout<<"mode index"<<idx<<std::endl;
			modes->insert(ModeType(body_actions,idx));
		}
	}
}

devActions XMLMIDIParser::parseIO(rapidxml::xml_node<> *nodes)
{
	
	devActions ret;
	rapidxml::xml_attribute<> *in_type;
	in_type = nodes->first_attribute("type",4,true);

	if(!in_type)   //there must be a type
		return ret;
	
	char *type = in_type->value();
	
	if(!strncmp(type, "mouse",5))
	{
		ret.tp = mouse;
		rapidxml::xml_attribute<> *xml_dx = nodes->first_attribute("dx",2,true);
		if(xml_dx)
		{
			char *dx = xml_dx->value();
			
			if(dx)
			{
			ret.mouse.dx = atoi(dx);
			}
		}
		rapidxml::xml_attribute<> *xml_dy = nodes->first_attribute("dy",2,true);
		if(xml_dy)
		{
		char *dy = xml_dy->value();
			if(dy)
			{
				ret.mouse.dy = atoi(dy);
			}		
		}	


		rapidxml::xml_attribute<> *xml_wm = nodes->first_attribute("wheel_move",10,true);
		if(xml_dy)
		{
		char *wheel_move = xml_wm->value();
			if(wheel_move)
			{
				ret.mouse.wheel_move = atoi(wheel_move);
			}		
		}	

		
		rapidxml::xml_attribute<> *xml_gotox = nodes->first_attribute("gotox",5,true);
		if(xml_gotox){
			char *gotox = xml_gotox->value();
			if(gotox)
			{
				ret.mouse.gotox = atoi(gotox);
			}
		}
		rapidxml::xml_attribute<> *xml_gotoy = nodes->first_attribute("gotoy",5,true);
		if(xml_gotoy)
		{
			char *gotoy = xml_gotoy->value();
			if(gotoy)
			{
				ret.mouse.gotoy = atoi(gotoy);
			}
		}
		rapidxml::xml_attribute<> *xml_click = nodes->first_attribute("click",5,true);
		if(xml_click)
		{
			char *click = xml_click->value();
			if(click)
			{
				ret.mouse.click = atoi(click);
			}
		}
		rapidxml::xml_attribute<> *xml_rc = nodes->first_attribute("right_click",11,true);
		if(xml_rc)
		{
			char *right_click = xml_rc->value();
			if(right_click)
			{
				ret.mouse.right_click = atoi(right_click);
			}
		}
		rapidxml::xml_attribute<> *xml_delay = nodes->first_attribute("delay",6,true);
		if(xml_delay)
		{
			char *delay = xml_delay->value();
			if(delay)
			{
				ret.delay = atoi(delay);
			}
		}
	}
	else if(!strncmp(type, "keyboard",8))
	{
		ret.tp = keyboard;
		rapidxml::xml_attribute<> *kbData = nodes->first_attribute("data",4,true);
		rapidxml::xml_attribute<> *kbType = nodes->first_attribute("kbType",4,true);
		rapidxml::xml_attribute<> *kbDelay = nodes->first_attribute("delay",4,true);
		keyboardActions kbAct;
		
		if(kbData)
		{
			kbAct.data = std::string(kbData->value(),kbData->value_size());
		}
		else
		{
			kbAct.data = "";
		}
			
		if(kbType)
		{
			std::string kbt(kbType->value(),kbType->value_size());
			if(!kbt.compare("oneKey"))
				kbAct.type = oneKey;
			if(!kbt.compare("text"))
				kbAct.type = text;
			if(!kbt.compare("hotkey"))
				kbAct.type = hotkey;
		}
		else
		{
			kbAct.type = oneKey;
		}

		if(kbDelay)
		{
			kbAct.delay = atoi(kbDelay->value());
		}
		else
		{
			kbAct.delay = 0;
		}

		ret.kData = kbAct;
		std::cout<<"keyboard"<<kbAct.data<<"delay: "<<kbAct.delay<<"type:"<<kbAct.type<<std::endl;

	} 
	else if(!strncmp(type, "midi",4))
	{
		ret.tp = midi;
		ret.midi.byte[0] = 0;
		ret.midi.byte[1] = 0;
		ret.midi.byte[2] = 0;
		char *ptr; //do not use
		if (nodes->first_attribute("b0", 2, true))
		{
			ret.midi.byte[0] = (unsigned char)strtol(nodes->first_attribute("b0", 2, true)->value(),NULL, 16);
		}

		if (nodes->first_attribute("b1", 2, true))
		{
			ret.midi.byte[1] = (unsigned char)strtol(nodes->first_attribute("b1", 2, true)->value(),NULL, 16);
		}

		if (nodes->first_attribute("b2", 2, true))
		{
			ret.midi.byte[2] = (unsigned char)strtol(nodes->first_attribute("b2", 2, true)->value(),NULL, 16);
		}
	}
	return ret;
}

void XMLMIDIParser::ProcessHeader(rapidxml::xml_node<> *Header)
{
	std::cout<<"ProcessHeader"<<std::endl;
	if (Header)
	{
		
		for (rapidxml::xml_node<>* action_nodes = Header->first_node("action", 6, true)
			; action_nodes
			; action_nodes = action_nodes->next_sibling("action", 6, true))
		{
			rapidxml::xml_node<>* in_nodes = action_nodes->first_node("input",5,true);
			Actions action;
			if(in_nodes)
			{
				action.in = parseIO(in_nodes);
			}

			for (rapidxml::xml_node<>* out_nodes = action_nodes->first_node("output", 6, true)
				; out_nodes
				; out_nodes = out_nodes->next_sibling("output", 6, true))
			{
				action.out.push_back(parseIO(out_nodes));
			}
			
			header_actions->push_back(action);
		}
	}
}

bool XMLMIDIParser::Initializer()
{
	std::cout<<"Initializer"<<std::endl;
	bool ret = false;
	rapidxml::xml_node<> *Device = xmlDoc.first_node("DEVICE", 6, true);
	if(Device)
	{
		if (Device->first_attribute("type", 4, true))
		{
			char *dType = Device->first_attribute("type", 4, true)->value(); 
			if(dType){
				if(!strncmp(dType,"midi",4)){
					type=midi;
				}else if(!strncmp(dType,"keyboard",8)){
					type=keyboard;
				}else if(!strncmp(dType,"mouse",5)){
					type=mouse;
				}
			}
		}
		if (Device->first_attribute("timeout", 7, true))
		{
			timeout = (unsigned int) atoi(Device->first_attribute("timeout", 7, true)->value());
		}

		
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
		
		rapidxml::xml_node<> *Header = Device->first_node("header", 6, true);
		
		if(Header)
			ProcessHeader(Header);

		rapidxml::xml_node<> *Body = Device->first_node("body", 4, true);
		if(Body)
			ProcessMainBody(Body);
			else
			{
				std::cout<<"body does not exist"<<std::endl;
			}
		ret = true;
	}
	return ret;
}

bool XMLMIDIParser::loadFile(const std::string &filename) {
	bool ret = false;
	std::cout<<"loadFile"<<std::endl;
	std::ifstream is;
	is.open(filename, std::ifstream::binary | std::ifstream::ate);
	
	if (is.is_open()) {
		
		ret = true;
		raw_xml.resize(static_cast<size_t>(is.tellg()));
		is.seekg(0);
		is.read(raw_xml.data(), raw_xml.size());
		raw_xml.push_back(0);
	}
	std::cout<<"loadFile:"<<ret<<std::endl;
	return ret;
}


