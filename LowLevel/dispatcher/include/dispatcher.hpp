#ifndef DISPATCHER_H
#define DISPATCHER_H
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include "zmq.hpp"
#include <map>
#include <string>
#include "configParser.hpp"
#include "uuid/uuid.h"
#include <mutex>
#include <pistache/description.h>
#include <pistache/endpoint.h>
#include <pistache/serializer/rapidjson.h>
#include <pistache/http.h>

using namespace Pistache;




class dispatcher{
    private:
        config disp;
        std::string FileName;
        

        std::atomic_bool stop;
        

        zmq::context_t io_context{1};
        zmq::socket_t io_socket{io_context, zmq::socket_type::pull};
        void th_io();


        zmq::context_t coms_context{1};
        zmq::socket_t coms_socket{coms_context, zmq::socket_type::rep};
        
        void th_heart_beat();

        void http_start();
        Pistache::Address Addr;
        std::shared_ptr<Http::Endpoint> httpEndpoint;
        Rest::Description desc;
        Rest::Router router;
        void PostCommand(const Rest::Request &req, Http::ResponseWriter writer);
        void GetConfigs(const Rest::Request &req, Http::ResponseWriter writer);
        void init(size_t _threads, int _maxRequestSize);
        void start();
        void setupRoutes();


        std::map<std::string, std::string> devices;
        std::mutex lock_devices;
        zmq::context_t st_context{1};
        zmq::socket_t st_socket{st_context, zmq::socket_type::rep};
        void th_unique_number();
        std::string generate_unique_number(std::string l_devname);
        


        std::vector<std::thread> vth;




        void startup();
    public:
        void die();
        dispatcher();
        dispatcher(std::string fileName, Pistache::Address addr, std::atomic_bool *stop);
        ~dispatcher();
};

#endif 