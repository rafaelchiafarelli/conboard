#pragma once

#include <iostream>
#include <string>
#include <zmq.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <queue>
#define WAIT_FOR_TRIES 100000
#define STACKED_IO_MSG 10
class zmq_coms{
    
    private:
        bool hb_connected = false;
        bool io_connected = false;
        bool un_connected = false;
        std::atomic_bool stop;

        std::string DevName;
        
        std::string hb_address;
        zmq::context_t hb_context{1};
        zmq::socket_t hb_socket{hb_context, zmq::socket_type::req};
        void heartbeat_handler();

        std::string io_address;
        zmq::context_t io_context{1};
        zmq::socket_t io_socket{io_context, zmq::socket_type::req};
        std::thread *io_thread; 
        std::mutex io_mu;
        std::queue<std::string> io_queue;
        void th_io();
        void io_handler();

        std::string un_address;        
        std::string unique_number;
        zmq::context_t un_context{1};
        zmq::socket_t un_socket{un_context, zmq::socket_type::req};
        void unique_number_handler();

        std::vector<std::string> explode(std::string const & s, char delim);
    public:
        void die(){
            stop = true;
            io_thread->join();
        }
       std::vector<std::string> heartbeat();
       bool dispatch(std::string msg);
        zmq_coms(std::string devName, 
                    std::string _un_address, 
                    std::string _io_address,
                    std::string _hb_address);
        ~zmq_coms();

};