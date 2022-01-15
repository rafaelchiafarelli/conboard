#pragma once
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

class coms_type{
  public:
    std::string address = "";
    int port = -1;
};

class io_type{
  public:
    std::string address = "";
    int port = -1;
    unsigned int History;
};

class http_config{
  public:
    int threads = -1;
    int maxsize = -1;
    std::string ConfigAddr = "/config";
    std::string IOCommandAddr = "/iocommand";

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


class config{
    private:
      std::string data;
      ParseResult res;
      Document Doc;

      bool GetZMQio(io_type *z);
      bool GetZMQcoms(coms_type *z);
      bool GetHTTP(http_config *h);
      bool GetUUID_cfg(UUID_config *u);
      bool GetKey_cfg(key_config *k);
      bool GetScreen(screen_config *s);

    public:
      io_type  io;
      bool io_parse;
      coms_type coms;
      bool coms_parse;
      http_config http;
      bool http_parse;
      UUID_config cfgUUID;
      bool cfgUUID_parse;
      key_config cfgKey;
      bool cfgKey_parse;
      screen_config screen;
      bool screen_parse;



      std::string GetConfig(){return data;};
      config(std::string fileName);
      
      ~config(){};
};
