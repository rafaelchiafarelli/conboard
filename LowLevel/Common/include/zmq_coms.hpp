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
// Bounds how long a REQ socket's recv() blocks waiting for a reply. Without this,
// a dead/unreachable dispatcher (e.g. stopped first during a reinstall) leaves the
// heartbeat/io thread blocked forever, so Stop()/stopEngine() can never join it --
// this is what caused conKeyB/conMouse to hang ~90s on SIGTERM and get SIGKILLed.
#define RCV_TIMEOUT_MS 1000
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
        std::string unique_number = "";
        zmq::context_t un_context{1};
        zmq::socket_t un_socket{un_context, zmq::socket_type::req};
        void unique_number_handler();

        std::vector<std::string> explode(std::string const & s, char delim);
    public:
        // DeviceEngine::stopEngine() calls die() explicitly and then `delete com`,
        // whose destructor calls die() again -- join() must tolerate that second
        // call. std::thread::join() on an already-joined thread is undefined
        // behaviour (libstdc++ throws std::system_error("Invalid argument")); guard
        // with joinable() so this is idempotent.
        void die(){
            stop = true;
            if (io_thread && io_thread->joinable())
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