#pragma once

#include <iostream>
#include <string>
#include <zmq.hpp>
#include <atomic>
#include <thread>

class zmq_coms{
    
    private:
            /*zmq related functions and variables*/
        
        std::string unique_number;
        std::string DevName;
        void local_connect();
        zmq::context_t coms_context{1};
        zmq::socket_t coms_socket{coms_context, zmq::socket_type::req};

        zmq::context_t st_context{1};
        zmq::socket_t st_socket{st_context, zmq::socket_type::req};

        std::vector<std::string> explode(std::string const & s, char delim);
    public:
       std::vector<std::string> heartbeat();
        zmq_coms(std::string devName);
        ~zmq_coms();

};