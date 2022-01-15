#include "configParser.hpp"

config::config(std::string fileName)
{
    std::ifstream file(fileName);
	std::stringstream buff;
	buff<<file.rdbuf();
	data = "";
	data = buff.str();
	res = Doc.Parse(data.c_str());
    
    std::cout<<"parse result:"<<res.IsError()<<std::endl;
    io_parse = GetZMQio(&io);
    coms_parse = GetZMQcoms(&coms);
    http_parse = GetHTTP(&http);
    cfgUUID_parse = GetUUID_cfg(&cfgUUID);
    cfgKey_parse = GetKey_cfg(&cfgKey);

}

bool config::GetZMQio(io_type *z)
{
    bool ret = !res.IsError();
    if(ret)
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
                if(Doc["zmq_io"]["address"].IsString())
                {
                    io.address = Doc["zmq_io"]["address"].GetString();
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
            if(Doc["zmq_io"].HasMember("history"))
            {
                if(Doc["zmq_io"]["history"].IsInt())
                {
                    io.History = (unsigned int)Doc["zmq_io"]["history"].GetInt();
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

bool config::GetZMQcoms(coms_type *z)
{
    bool ret = !res.IsError();
    std::cout<<"GetZMQcoms - ret"<<ret<<std::endl;
    if(ret)
    {
        std::cout<<"GetZMQcoms - ok"<<std::endl;

        if(Doc.HasMember("zmq_hb"))
        {
            std::cout<<"GetZMQcoms - port"<<std::endl;
            if(Doc["zmq_hb"].HasMember("port"))
            {
                std::cout<<"GetZMQcoms - getport"<<std::endl;
                if(Doc["zmq_hb"]["port"].IsInt())
                {
                    coms.port = Doc["zmq_hb"]["port"].GetInt();
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
            std::cout<<"GetZMQcoms - address"<<std::endl;
            if(Doc["zmq_hb"].HasMember("address"))
            {
                std::cout<<"GetZMQcoms - get address"<<std::endl;
                if(Doc["zmq_hb"]["address"].IsString())
                {
                    coms.address = std::string(Doc["zmq_hb"]["address"].GetString());
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
    *z = coms;
    return ret;
}

bool config::GetHTTP(http_config *h)
{
    bool ret = !res.IsError();
    std::cout<<"GetHTTP - ok"<<std::endl;
    if(ret)
    {
        std::cout<<"GetHTTP - http"<<std::endl;
        if(Doc.HasMember("http"))
        {
            std::cout<<"GetHTTP - port"<<std::endl;
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
            std::cout<<"GetHTTP - config"<<std::endl;
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
            std::cout<<"GetHTTP - iocommand"<<std::endl;
            if(Doc["http"].HasMember("IOCommandAddr"))
            {
                if(Doc["http"]["IOCommandAddr"].IsString())
                {
                    http.IOCommandAddr = Doc["http"]["IOCommandAddr"].GetString();
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
            std::cout<<"GetHTTP - threads"<<std::endl;
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
            std::cout<<"GetHTTP - maxsize"<<std::endl;
            if(Doc["http"].HasMember("maxsize"))
            {
                if(Doc["http"]["maxsize"].IsInt())
                {
                    http.maxsize = Doc["http"]["maxsize"].GetInt();
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
    std::cout<<"GetUUID_cfg - ok"<<std::endl;
    if(ret)
    {
        std::cout<<"GetUUID_cfg - UUID"<<std::endl;
        if(Doc.HasMember("zmq_un"))
        {
            std::cout<<"GetUUID_cfg - address"<<std::endl;
            if(Doc["zmq_un"].HasMember("address"))
            {
                if(Doc["zmq_un"]["address"].IsString())
                {
                    cfgUUID.address = Doc["zmq_un"]["address"].GetString();
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
            std::cout<<"GetUUID_cfg - port"<<std::endl;
            if(Doc["zmq_un"].HasMember("port"))
            {
                if(Doc["zmq_un"]["port"].IsInt())
                {
                    cfgUUID.port = Doc["zmq_un"]["port"].GetInt();
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
    if(ret)
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
    if(ret)
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


