
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <set>
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "jsonParser.h"

using namespace rapidjson;


jsonParser::jsonParser(const char *_FileName, std::vector<ModeType> *Mode,std::vector<Actions> *h) {
	
	header_actions = h;
	op_modes = Mode;
	loaded = false;
	FileName = _FileName;
	Reload();	
	
}

void jsonParser::Reload(){
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

jsonParser::~jsonParser()
{

}

void jsonParser::ProcessMainBody(rapidjson::Value &body)
{
	if(body.HasMember("modes"))
	{
		rapidjson:Value& modes = body["modes"];
		if(modes.IsArray())
		{
			for(SizeType i = 0;
				i<modes.Size();
				i++)
			{
				ModeType mode;
				rapidjson::Value& jMode = modes[i];
				mode.index = -1;
				if(jMode.HasMember("id"))
				{
					if(jMode["id"].IsString())
						mode.index = jMode["id"].GetInt();
				}
				//std::cout<<"id done"<<std::endl;

				mode.is_active = false;
				if(jMode.HasMember("active"))
				{
					if(jMode["active"].IsBool())
						mode.is_active = jMode["active"].GetBool();
				}
				//std::cout<<"active done"<<std::endl;

				if(jMode.HasMember("mode_header"))
				{
					rapidjson::Value& mode_header = jMode["mode_header"];
					if(mode_header.HasMember("actions"))
					{
						rapidjson::Value& actions = mode_header["actions"];
						if(actions.IsArray())
						{
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
				}
				//std::cout<<"mode_header done"<<std::endl;
				if(jMode.HasMember("actions"))
				{
					rapidjson::Value& bActions = jMode["actions"];
					std::cout<<"actions IsArray:"<<bActions.IsArray()<<std::endl;
					if(bActions.IsArray())
					{
						std::cout<<"actions.size"<<bActions.Size()<<std::endl;
						for(SizeType t = 0;
							t<bActions.Size();
							t++)
						{
							Actions mActions;
							rapidjson::Value& Act = bActions[t];
							std::cout<<"has input:"<<Act.HasMember("input")<<std::endl;
							if(Act.HasMember("input"))
							{
								mActions.in = parseIO(Act["input"]);
								std::cout<<"input:"<<mActions.in.mAct<<std::endl;
							}
							std::cout<<"has output:"<<Act.HasMember("output")<<std::endl;
							if(Act.HasMember("output"))
							{
								devActions dAct;
								rapidjson::Value& out = Act["output"];
								std::cout<<"out IsArray"<<out.IsArray()<<std::endl;
								if(out.IsArray())
								{
									std::cout<<"output size:"<<out.Size()<<std::endl;
									for(SizeType j = 0;
										j < out.Size();
										j++)
									{
										dAct = parseIO(out[j]);
										std::cout<<"action out:"<<dAct<<std::endl;
										mActions.out.push_back(dAct);
									}
								}
							}
							mode.body_actions.push_back(mActions);
						}
					}
				}
				//std::cout<<"actions done"<<std::endl;
				op_modes->push_back(mode);
			}
		}
	}
}

devActions jsonParser::parseIO(rapidjson::Value& act)
{
	devActions ret;
	if(act.HasMember("type"))
	{
		std::string tp = "";
		if(act["type"].IsString())
			tp = (act["type"].GetString());		
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
				if(act["data"].IsString())
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
				std::string keyType = "";
				if(act.IsString())
					keyType = act["keyType"].GetString();

				if(keyType.compare("hotKey"))
				{
					ret.kData.type = keyType::hotkey;
				}
				else if(keyType.compare("text"))
				{
					ret.kData.type = keyType::text;
				}
			}
			//if it is not_hold or not present or empty, it is not_hold
			ret.kData.hold = holdType::not_hold;
			if(act.HasMember("hold"))
			{
				std::string Hold = "";
				if(act["hold"].IsString())
					Hold = act["hold"].GetString();

				if(Hold.compare("hold"))
				{
					ret.kData.hold = holdType::hold;
				}
				else if(Hold.compare("hold_delay"))
				{
					ret.kData.hold = holdType::hold_delay;
				}
			}
			if(act.HasMember("delay"))
			{
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

void jsonParser::ProcessHeader(rapidjson::Value& header)
{
	if(header.HasMember("identifier"))
	{
		rapidjson::Value& identifier = header["identifier"];
		if(identifier.HasMember("generics"))
		{
			rapidjson::Value& generics = identifier["generics"];
			if(generics.IsArray())
			{
				for (Value::ConstMemberIterator iter = generics.MemberBegin(); iter != generics.MemberEnd(); ++iter)
				{
					KeyValue kv;
					
					kv.key = iter->name.GetString();
					kv.value = iter->value.GetString();
					Generics.push_back(kv);
				}
			}
		}
		//std::cout<<"generics done"<<std::endl;
		if(identifier.HasMember("tags"))
		{
			rapidjson::Value& tags = identifier["tags"];
			if(tags.IsArray())
			{
				for (Value::ConstMemberIterator iter = tags.MemberBegin(); iter != tags.MemberEnd(); ++iter)
				{
					KeyValue kv;
					kv.key = iter->name.GetString();
					kv.value = iter->value.GetString();
					Tags.push_back(kv);
				}
			}
		}
		//std::cout<<"tags done"<<std::endl;
		if(identifier.HasMember("executable"))
		{
			rapidjson::Value& executable = identifier["executable"];
			if(executable.HasMember("exec"))
			{
				Ex.exec = "";
				if(executable["exec"].IsString())
					Ex.exec = executable["exec"].GetString();
			}
			//std::cout<<"exec done"<<std::endl;
			if(executable.HasMember("count"))
			{
				Ex.param_count = 0;
				if(executable["count"].IsInt())
					Ex.param_count= executable["count"].GetInt();
			}
			//std::cout<<"count done"<<std::endl;
			if(executable.HasMember("params"))
			{
				rapidjson::Value& params = executable["params"];
				if(params.IsArray()){
					for (Value::ConstMemberIterator iter = params.MemberBegin(); iter != params.MemberEnd(); ++iter)
					{
						KeyValue kv;
						if(iter->name.IsString())
							kv.key = iter->name.GetString();
						if(iter->value.IsString())
							kv.value = iter->value.GetString();
						Ex.params.push_back(kv);
					}
				}
			}
			//std::cout<<"params done"<<std::endl;
		}
		//std::cout<<"executables done"<<std::endl;
	}
	if(header.HasMember("actions"))
	{
		Actions action;
		rapidjson::Value& actions = header["actions"];
		if(actions.IsArray())
		{
			for(SizeType t; t<actions.Size();t++)
			{
				action.out.push_back(parseIO(actions[t]));
			}
		}
		header_actions->push_back(action);
	}
	//std::cout<<"actions done"<<std::endl;
}

devType jsonParser::GetDevType(std::string dType)
{
	devType ltype = devType::notype;
	std::cout<<"Type: "<<dType<<std::endl;
	if(!dType.compare("midi"))
		ltype = devType::midi;
	else if(!dType.compare("keyboard"))
		ltype = devType::keyboard;
	else if(!dType.compare("mouse"))
		ltype = devType::mouse;
	else if(!dType.compare("joystick"))
		ltype = devType::joystick;
	else if(!dType.compare("notype"))
		ltype = devType::notype;
	return ltype;
}

bool jsonParser::Initializer()
{
	
	bool ret = false;
		
	if(Doc.HasMember("DEVICE"))
	{
		//std::cout<<"here"<<std::endl;
		rapidjson::Value& Device = Doc["DEVICE"];

		if (Device.HasMember("type"))
		{
			if(Device["type"].IsString())
			{
				std::string dType = Device["type"].GetString();
				type = GetDevType(dType);
			}
			else
			{
				type = devType::notype;
			}
		}
		else
		{
			type = devType::notype;
		}

		if (Device.HasMember("timeout"))
		{
			if(Device["timeout"].IsInt())
				timeout = (unsigned int ) Device["timeout"].GetInt();
			else
			{
				timeout = 0;
			}
		}
		else
		{
			timeout = 0;
		}

		if (Device.HasMember("name"))
		{
			if(Device["name"].IsString())
				jsonParser::DevName = Device["name"].GetString();
			else
				jsonParser::DevName = "";
		}
		else
		{
			jsonParser::DevName = "";
		}
		
		if (Device.HasMember("input"))
		{
			if(Device["input"].IsString())
				jsonParser::DevInput = Device["input"].GetString();
			else
				jsonParser::DevInput = "";
		}
		else
		{
			jsonParser::DevInput = "";
		}   
		
		if (Device.HasMember("output"))
		{
			if(Device["output"].IsString())
				jsonParser::DevOutput = Device["output"].GetString();
			else
				jsonParser::DevOutput = "";
		}
		else
		{
			jsonParser::DevOutput = "";
		}
	ret = true;
	}
	//std::cout<<"device done"<<std::endl;
	if(Doc.HasMember("header"))
	{

		rapidjson::Value& header = Doc["header"];
		ProcessHeader(header);
		//std::cout<<"header done"<<std::endl;
	}
	if(Doc.HasMember("body"))
	{
		rapidjson::Value& body = Doc["body"];
		ProcessMainBody(body);
		//std::cout<<"body done"<<std::endl;
	}
	return ret;
}

bool jsonParser::loadFile(const char *filename)
{
	jsonParser::FileName = std::string(filename);
	std::ifstream file(FileName);
	data<<file.rdbuf();
	ParseResult res = Doc.Parse(data.str());
	//std::cout<<"oia:"<<res.IsError()<<std::endl;
	return (res.IsError())? false:true;
}


