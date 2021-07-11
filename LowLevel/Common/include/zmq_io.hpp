#pragma once

#include <iostream>
#include <string>
#include <zmq.hpp>

class zmq_io{
    
    private:
            /*zmq related functions and variables*/
        zmq::context_t io_context{1};
        zmq::socket_t io_socket{io_context, zmq::socket_type::push};
        std::string devName;
        std::string unique_number;

    public:
        zmq::send_result_t send(std::string snd_data);
        zmq_io(std::string devName, std::string unique_number);
        ~zmq_io();

};