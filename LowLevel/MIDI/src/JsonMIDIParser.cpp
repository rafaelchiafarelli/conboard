
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <set>
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "JsonMIDIParser.h"

using namespace rapidjson;

JsonMIDIParser::JsonMIDIParser(const char *_FileName, std::vector<ModeType> *Mode,std::vector<Actions> *h) {
	
	header_actions = h;
	modes = Mode;
	loaded = false;
	FileName = _FileName;
	
	Reload();
	
	
}

void JsonMIDIParser::Reload(){
	if(loaded == false)
	{
		if (loadFile(FileName.c_str())) 
		{
	
			if (Doc.HasMember("DEVICE"))
			{
				loaded = Initializer();
			}
		}
	}
}

JsonMIDIParser::~JsonMIDIParser()
{

}

void JsonMIDIParser::ProcessMainBody(rapidjson::Value &body)
{
	if(body.HasMember("modes"))
	{
		if(body["modes"].IsArray())
		{
			for(SizeType i = 0;
			i<body.Size();
			i++)
			{
				ModeType mode;
				rapidjson::Value& jMode = body[i];
				if(jMode.HasMember("id"))
				{
					mode.index = jMode["id"].GetInt();
				}
				else
				{
					mode.index = -1;
				}
				if(jMode.HasMember("active"))
				{
					mode.is_active = jMode["active"].GetBool();
				}
				else
				{
					mode.is_active = false;
				}
				if(jMode.HasMember("mode_header"))
				{
					rapidjson::Value& mode_header = jMode["mode_header"];
					if(mode_header.HasMember("actions"))
					{
						rapidjson::Value& actions = mode_header["actions"];
						Actions mhActions;
						for(SizeType t=0;
						t<actions.Size();
						t++)
						{
							rapidjson::Value& act = actions[t];
							devActions dAct = parseIO(act);
							mhActions.out.push_back(dAct);
						}
						mode.header.push_back(mhActions);
					}
				}
				if(jMode.HasMember("actions"))
				{
					rapidjson::Value& bActions = jMode["actions"];
					for(SizeType t = 0;
					t<bActions.Size();
					t++)
					{
						Actions mActions;
						rapidjson::Value& Act = bActions[t];
						if(Act.HasMember("input"))
						{
							mActions.in = parseIO(Act["input"]);
						}
						if(Act.HasMember("output"))
						{
							devActions dAct;
							rapidjson::Value& out = Act["output"];
							for(SizeType j = 0;
							j < Act.Size();
							j++)
							{
								dAct = parseIO(out[j]);
								mActions.out.push_back(dAct);
							}
						}
						mode.body_actions.push_back(mActions);
					}
				}
			}
		}
	}
}

devActions JsonMIDIParser::parseIO(rapidjson::Value& act)
{
	
	devActions ret;
	

	if(act.HasMember("type"))
	{
		std::string tp(act["type"].GetString());
		ret.tp = GetDevType(tp);
	}
	switch(ret.tp)
	{
		case devType::mouse:
		{
			if(act.HasMember("dx"))
			{
				if(act["dx"].IsInt())
					ret.mouse.dx = act["dx"].GetInt();
				else
					ret.mouse.dx = 0;
			}
			else
			{
				ret.mouse.dx=0;
			}
			if(act.HasMember("dy"))
			{
				if(act["dy"].IsInt())
					ret.mouse.dy = act["dy"].GetInt();
				else
					ret.mouse.dy = 0;
			}
			else
			{
				ret.mouse.dy=0;
			}			
			if(act.HasMember("wheel_move"))
			{
				if(act["wheel_move"].IsInt())
					ret.mouse.wheel_move = act["wheel_move"].GetInt();
				else
					ret.mouse.wheel_move = 0;
			}
			else
			{
				ret.mouse.wheel_move=0;
			}			
			if(act.HasMember("gotox"))
			{
				if(act["gotox"].IsInt())
					ret.mouse.gotox = act["gotox"].GetInt();
				else
					ret.mouse.gotox = 0;
			}
			else
			{
				ret.mouse.gotox=0;
			}			
			if(act.HasMember("gotoy"))
			{
				if(act["gotoy"].IsInt())
					ret.mouse.gotoy = act["gotoy"].GetInt();
				else
					ret.mouse.gotoy = 0;
			}
			else
			{
				ret.mouse.gotoy=0;
			}	
			if(act.HasMember("click"))
			{
				if(act["click"].IsInt())
					ret.mouse.click = act["click"].GetInt();
				else
					ret.mouse.click = 0;
			}
			else
			{
				ret.mouse.click=0;
			}	
			if(act.HasMember("right_click"))
			{
				if(act["right_click"].IsInt())
					ret.mouse.right_click = act["right_click"].GetInt();
				else
					ret.mouse.right_click = 0;
			}
			else
			{
				ret.mouse.right_click=0;
			}		
			if(act.HasMember("delay"))
			{
				if(act["delay"].IsInt())
					ret.mouse.delay = act["delay"].GetInt();
				else
					ret.mouse.delay = 0;
			}
			else
			{
				ret.mouse.delay=0;
			}											
		}
		break;
		case devType::keyboard:
		{
			if(act.HasMember("data"))
			{
				ret.kData.data = act["data"].GetString();
			}
			else
			{
				ret.kData.data = "";
			}
			//if not present or not hotkey nor text, it is oneKey
			ret.kData.type = keyType::oneKey;
			if(act.HasMember("keyType"))
			{
				std::string keyType = act["keyType"].GetString();

				if(keyType.compare("hotKey"))
				{
					ret.kData.type = keyType::hotkey;
				}
				if(keyType.compare("text"))
				{
					ret.kData.type = keyType::text;
				}
			}
			//if it is not_hold or not present or empty, it is not_hold
			ret.kData.hold = holdType::not_hold;
			if(act.HasMember("hold"))
			{
				std::string Hold = act["hold"].GetString();
				if(Hold.compare("hold"))
				{
					ret.kData.hold = holdType::hold;
				}
				if(Hold.compare("hold_delay"))
				{
					ret.kData.hold = holdType::hold_delay;
				}
			}
			if(act.HasMember("delay"))
			{
				std::string Delay = act["delay"].GetString();
				ret.kData.delay = 0;
				if(act["delay"].IsInt())
				{
					ret.kData.delay = (unsigned int) act["delay"].GetInt();
				}
			}
		}
		break;
		case devType::midi:
		{
			if(act.HasMember("b0"))
			{
				ret.mAct.midi.byte[0] = 0;
				ret.mAct.midi.byte[1] = 0;
				ret.mAct.midi.byte[2] = 0;
				if(act["b0"].IsInt())
					ret.mAct.midi.byte[0] = act["b0"].GetInt();
				if(act["b1"].IsInt())
					ret.mAct.midi.byte[1] = act["b1"].GetInt();
				if(act["b2"].IsInt())
					ret.mAct.midi.byte[2] = act["b2"].GetInt();
				
			}
		}
		break;
		case devType::joystick:
		break;
		case devType::notype:
		break;
	}
	

	return ret;
}

void JsonMIDIParser::ProcessHeader(rapidjson::Value& header)
{
	if(header.HasMember("actions"))
	{
		Actions action;
		rapidjson::Value& actions = header["actions"];
		for(SizeType t; t<actions.Size();t++)
		{
			action.out.push_back(parseIO(actions[t]));
		}
		header_actions->push_back(action);
	}
}

devType JsonMIDIParser::GetDevType(std::string dType)
{
	devType ltype = devType::notype;
	if(dType.compare("midi"))
		ltype = devType::midi;
	if(dType.compare("keyboard"))
		ltype = devType::keyboard;
	if(dType.compare("mouse"))
		ltype = devType::mouse;
	if(dType.compare("joystick"))
		ltype = devType::joystick;
	if(dType.compare("notype"))
		ltype = devType::notype;
	return ltype;
}

bool JsonMIDIParser::Initializer()
{
	
	bool ret = false;

	if(Doc.HasMember("DEVICE"))
	{
		rapidjson::Value& Device = Doc["DEVICE"];

		if (Device.HasMember("type"))
		{
			std::string dType = Device["type"].GetString();

		}
		else
		{
			type = devType::notype;
		}
		if (Device.HasMember("timeout"))
		{
			if(Device["timeout"].IsInt())
				timeout = (unsigned int ) Device["timeout"].GetInt();
			//timeout = (unsigned int) atoi(Device->first_attribute("timeout", 7, true)->value());
		}
		else
		{
			timeout = 0;
		}

		if (Device.HasMember("name"))
		{
			JsonMIDIParser::DevName = Device["name"].GetString();
		}
		else
		{
			JsonMIDIParser::DevName = "";
		}
		
		if (Device.HasMember("input"))
		{
			JsonMIDIParser::DevInput = Device["input"].GetString();
		}
		else
		{
			JsonMIDIParser::DevInput = "";
		}   
		
		if (Device.HasMember("output"))
		{
			JsonMIDIParser::DevOutput = Device["output"].GetString();
		}
		else
		{
			JsonMIDIParser::DevOutput = "";
		}
	ret = true;
	}
	if(Doc.HasMember("header"))
	{
		rapidjson::Value& header = Doc["header"];
		ProcessHeader(header);
	}
	if(Doc.HasMember("body"))
	{
		rapidjson::Value& body = Doc["body"];
		ProcessMainBody(body);
	}
	

	return ret;
}

bool JsonMIDIParser::loadFile(const char *filename) {
	
	JsonMIDIParser::filename = std::string(filename);
	std::ifstream file(filename);
	data<<file.rdbuf();
	ParseResult res = Doc.Parse(data.str());
	return res.IsError();
}


