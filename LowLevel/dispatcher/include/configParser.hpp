#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include "stdio.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

#include <set>
#include <vector>
#include <map>

#include <iostream>
#include "list"
#include <cstddef>
#include <cassert>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

class zmq_coms{
  public:
    std::string address = "";
    int port = -1;
};

class zmq_io{
  public:
    std::string address = "";
    int port = -1;
};

class http_config{
  public:
    int threads;
    std::string ConfigAddr = "/config";
    std::string CommandAddr = "/command";
    int port = 9999;
};
class UUID_config{
public:
    std::string address = "";
    int port = -1;

};
class key_config{
public:
    std::string secret = "";
    bool oauth = false;

};
class screen_config{
  public:
    std::string type = "";
    std::string coms = "";
    int adr = -1;
    std::vector<int> cmds;
    int lines = -1;
    int columns = -1;
    int colors = -1;

};
class vault_config{
  public:
    std::string path = "";
    std::string encription = "";
    std::string key_gen = "";
    std::string verify = "";
};
class shared_config{
  public:
    std::string path = "";
    std::string mount = "";
    std::string m_flags = "";
};


class config{
    private:
      std::string data;
      ParseResult res;
      Document Doc;
      zmq_io  io;
      zmq_coms coms;
      http_config http;
      UUID_config cfgUUID;
      key_config cfgKey;
      screen_config screen;
      vault_config vault;
      shared_config shared;

    public:
      bool GetZMQio(zmq_io *z);
      bool GetZMQcoms(zmq_coms *z);
      bool GetHTTP(http_config *h);
      bool GetUUID_cfg(UUID_config *u);
      bool GetKey_cfg(key_config *k);
      bool GetScreen(screen_config *s);
      bool GetVault(vault_config *v);
      bool GetShared(shared_config *sh);

      config(std::string fileName);
      
      ~config(){};
};

#endif