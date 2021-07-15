#include "configParser.hpp"

config::config(std::string fileName)
{
    std::ifstream file(fileName);
	std::stringstream buff;
	buff<<file.rdbuf();
	data = "";
	data = buff.str();
	ParseResult isError = Doc.Parse(data.c_str());


}

bool config::GetZMQio(zmq_io *z)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("zmq_io"))
        {
            if(Doc["zmq_io"].HasMember("port"))
            {
                if(Doc["zmq_io"]["port"].IsInt())
                {
                    io.port = Doc["zmq_io"]["port"].GetInt();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }            
            if(Doc["zmq_io"].HasMember("address"))
            {
                if(Doc["zmq_io"]["port"].IsString())
                {
                    io.address = Doc["zmq_io"]["port"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            
        }
    }
    *z = io;
    return ret;
}

bool config::GetZMQcoms(zmq_coms *z)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("zmq_coms"))
        {
            if(Doc["zmq_coms"].HasMember("port"))
            {
                if(Doc["zmq_coms"]["port"].IsInt())
                {
                    coms.port = Doc["zmq_coms"]["port"].GetInt();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }            
            if(Doc["zmq_coms"].HasMember("address"))
            {
                if(Doc["zmq_coms"]["port"].IsString())
                {
                    coms.address = Doc["zmq_coms"]["port"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            
        }
    }
    *z = coms;
    return ret;
}

bool config::GetHTTP(http_config *h)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("http"))
        {
            if(Doc["http"].HasMember("port"))
            {
                if(Doc["http"]["port"].IsInt())
                {
                    http.port = Doc["http"]["port"].GetInt();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }            
            if(Doc["http"].HasMember("ConfigAddr"))
            {
                if(Doc["http"]["ConfigAddr"].IsString())
                {
                    http.ConfigAddr = Doc["http"]["ConfigAddr"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["http"].HasMember("CommandAddr"))
            {
                if(Doc["http"]["CommandAddr"].IsString())
                {
                    http.CommandAddr = Doc["http"]["CommandAddr"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }            
            if(Doc["http"].HasMember("threads"))
            {
                if(Doc["http"]["threads"].IsInt())
                {
                    http.threads = Doc["http"]["threads"].GetInt();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }       
        }
    }
    *h = http;
    return ret;
}


bool config::GetUUID_cfg(UUID_config *u)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("UUID"))
        {
        
            if(Doc["UUID"].HasMember("address"))
            {
                if(Doc["UUID"]["address"].IsString())
                {
                    cfgUUID.address = Doc["UUID"]["address"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["UUID"].HasMember("port"))
            {
                if(Doc["UUID"]["port"].IsInt())
                {
                    cfgUUID.port = Doc["UUID"]["port"].GetInt();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
        }
    }
    *u = cfgUUID;
    return ret;
}

bool config::GetKey_cfg(key_config *k)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("key"))
        {
        
            if(Doc["key"].HasMember("secret"))
            {
                if(Doc["key"]["secret"].IsString())
                {
                    cfgKey.secret = Doc["key"]["secret"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            
            if(Doc["key"].HasMember("oauth"))
            {
                if(Doc["key"]["oauth"].IsBool())
                {
                    cfgKey.oauth = Doc["key"]["oauth"].GetBool();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
        }
    }
    *k = cfgKey;
    return ret;
}


bool config::GetScreen(screen_config *s)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("screen"))
        {
        
            if(Doc["screen"].HasMember("type"))
            {
                if(Doc["screen"]["type"].IsString())
                {
                    screen.type = Doc["screen"]["type"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            
            if(Doc["screen"].HasMember("coms"))
            {
                if(Doc["screen"]["coms"].IsString())
                {
                    screen.coms = Doc["screen"]["coms"].IsString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["screen"].HasMember("adr"))
            {
                if(Doc["screen"]["adr"].IsInt())
                {
                    screen.adr = Doc["screen"]["adr"].IsInt();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["screen"].HasMember("configs"))
            {
                if(Doc["screen"]["configs"].IsArray())
                {
                    for(Value::ConstValueIterator it = Doc["screen"]["configs"].Begin();
                        it != Doc["screen"]["configs"].End();
                        it++)
                        {
                            if(it->IsInt())
                            {
                                screen.cmds.push_back(it->GetInt());
                            }
                            else{
                                screen.cmds.push_back(-1);
                            }
                        }
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["screen"].HasMember("format"))
            {
                if(Doc["screen"]["format"].IsObject())
                {
                    if(Doc["screen"]["format"].HasMember("lines"))
                    {
                        if(Doc["screen"]["format"]["lines"].IsInt())
                        {
                            screen.lines = Doc["screen"]["format"]["lines"].GetInt();
                        }
                        else
                        {
                            ret = false;
                        }
                    }
                    else
                    {
                        ret = false;
                    }
                    if(Doc["screen"]["format"].HasMember("columns"))
                    {
                        if(Doc["screen"]["format"]["columns"].IsInt())
                        {
                            screen.columns = Doc["screen"]["format"]["columns"].GetInt();
                        }
                        else
                        {
                            ret = false;
                        }
                    }
                    else
                    {
                        ret = false;
                    }       
                    if(Doc["screen"]["format"].HasMember("colors"))
                    {
                        if(Doc["screen"]["format"]["colors"].IsInt())
                        {
                            screen.colors = Doc["screen"]["format"]["colors"].GetInt();
                        }
                        else
                        {
                            ret = false;
                        }
                    }
                    else
                    {
                        ret = false;
                    }                                   
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }

        }
    }

    *s = screen;
    return ret;
}




bool config::GetVault(vault_config *v)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("vault"))
        {
             if(Doc["vault"].HasMember("path"))
            {
                if(Doc["vault"]["path"].IsString())
                {
                    vault.path = Doc["vault"]["path"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["vault"].HasMember("encription"))
            {
                if(Doc["vault"]["encription"].IsString())
                {
                    vault.encription = Doc["vault"]["encription"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }            
            if(Doc["vault"].HasMember("key_gen"))
            {
                if(Doc["vault"]["key_gen"].IsString())
                {
                    vault.key_gen = Doc["vault"]["key_gen"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }  
            if(Doc["vault"].HasMember("verify"))
            {
                if(Doc["vault"]["verify"].IsString())
                {
                    vault.verify = Doc["vault"]["verify"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }              
        }
    }
    *v = vault;
    return ret;
}

bool config::GetShared(shared_config *sh)
{
    bool ret = !res.IsError();
    if(res.IsError() == true)
    {
        if(Doc.HasMember("shared"))
        {
             if(Doc["shared"].HasMember("path"))
            {
                if(Doc["shared"]["path"].IsString())
                {
                    shared.path = Doc["shared"]["path"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
            if(Doc["shared"].HasMember("mount"))
            {
                if(Doc["shared"]["mount"].IsString())
                {
                    shared.mount = Doc["shared"]["mount"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }            
            if(Doc["shared"].HasMember("m_flags"))
            {
                if(Doc["shared"]["m_flags"].IsString())
                {
                    shared.m_flags = Doc["shared"]["m_flags"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }  
            if(Doc["vault"].HasMember("verify"))
            {
                if(Doc["vault"]["verify"].IsString())
                {
                    vault.verify = Doc["vault"]["verify"].GetString();
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }              
        }
    }
    *sh = shared;
    return ret;
}