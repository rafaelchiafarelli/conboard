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

    //unique number context
        zmq::context_t un_context{1};
        zmq::socket_t un_socket{un_context, zmq::socket_type::rep};
        std::thread *th_unuique_numb;
        void th_unique_number();
        std::string generate_unique_number(std::string l_devname);

    //io (user actions) context
        zmq::context_t io_context{1};
        zmq::socket_t io_socket{io_context, zmq::socket_type::rep};
        std::thread *io;
        void th_io();
    
    //heartbeat context
        zmq::context_t coms_context{1};
        zmq::socket_t coms_socket{coms_context, zmq::socket_type::rep};
        std::thread *hb;
        void th_heart_beat();

    //http connection (get and post for config and command) and socket (outside world)
        void th_http();
        std::thread *http_com;

        void PostCommand(const Rest::Request &req, Http::ResponseWriter writer);
        void GetConfigs(const Rest::Request &req, Http::ResponseWriter writer);
        void setupRoutes();
        std::vector<std::string> explode(std::string const & s, char delim);
        std::map<std::string, std::string> devices; //<unique_number, devname>
        std::map<std::string, std::string> commands; //<unique_number, command>
        std::map<std::string, std::chrono::time_point<std::chrono::system_clock>>  last_ping;//<unique_number, last_ping_time_point>
        std::mutex lock_devices;
    
        void startup();

    public:
        void die();
        dispatcher();
        dispatcher(std::string fileName, std::atomic_bool *stop);
        ~dispatcher();
};

#endif 