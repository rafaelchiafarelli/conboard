#include "zmq_coms.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>

#include <string>

#include <iomanip>

#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <algorithm>
#include <unistd.h>

#include <chrono>

zmq_coms::~zmq_coms(){}

zmq_coms::zmq_coms(std::string devName, 
                    std::string _un_address, 
                    std::string _io_address,
                    std::string _hb_address)
{
    //connect to zmq servers
    
    DevName = devName;
    hb_address = _hb_address;
    io_address = _io_address;
    un_address = _un_address;

    unique_number_handler();
    heartbeat_handler();
    io_handler();
}

/**
 *  Thread that will send the io actions (user actions) to the server
 * 
 **/ 
void zmq_coms::th_io()
{
    bool send_io = false;
    std::string to_send = "";
    int tries = 0;
    while (!stop)
    {
        send_io = false;
        
        {
            std::lock_guard<std::mutex> locker(io_mu);
            if(!io_queue.empty())
            {
                to_send = io_queue.front();
                io_queue.pop();
                send_io = true;
            }
        }
        if(send_io && io_connected)
        {
            std::string msg="{\"DevName\":";
            msg.append(DevName);
            msg.append(",\"unique_number\":");
            msg.append(unique_number);
            msg.append(",\"msg\":");
            msg.append(to_send);
            msg.append("}");
            zmq::message_t request_msg(msg);
            zmq::send_result_t res_send = io_socket.send(request_msg, zmq::send_flags::dontwait);
            // Get the reply
            zmq::message_t recv_msg;
            zmq::recv_result_t res  = un_socket.recv(recv_msg,zmq::recv_flags::dontwait);
            std::string tmp((char *)recv_msg.data());
            std::cout<<"IO sent to user and from:"<<DevName<<" and respose was:"<<tmp<<std::endl;
            
        }
        else if (!io_connected)
        {
            tries+=1;
            if(tries > WAIT_FOR_TRIES){
                io_handler();
                tries = 0;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
bool zmq_coms::dispatch(std::string msg)
{
    bool ret = false;
    {
        std::lock_guard<std::mutex> locker(io_mu);
        if(io_queue.size()< STACKED_IO_MSG)
        {
            io_queue.push(msg);
            ret = true;
        }
    }
    return ret;
}
/**
 *  Connect to the io server
 * 
 **/ 
void zmq_coms::io_handler()
{
    io_connected = io_socket.connect(io_address);
    if(!io_connected)
        std::cout<<"IO server not connected"<<std::endl;
}
/**
 * connect to the heartbeat server
 **/ 
void zmq_coms::heartbeat_handler(){

    hb_connected = hb_socket.connect(hb_address);
    if(!hb_connected)
        std::cout<<"Heartbeat not connected"<<std::endl;
}
/**
 * connect to the unique number server
 **/ 
void zmq_coms::unique_number_handler(){
    // connect to the server and get a unique ID
        //send the DevName to the dispatcher and receive a unique number in response
        //there is a catch if two devices with the same name are installed. See dispatcher for more information
        

    un_connected = un_socket.connect(un_address);

    if(un_connected)
    {

        zmq::message_t request_msg(DevName);
        zmq::send_result_t res_send = un_socket.send(request_msg, zmq::send_flags::dontwait);
        // Get the reply
        zmq::message_t recv_msg;
        zmq::recv_result_t res  = un_socket.recv(recv_msg,zmq::recv_flags::none);
        std::string tmp((char *)recv_msg.data());
        unique_number = tmp;
        std::cout<<"DevName:"<<DevName<<"unique number:"<<unique_number<<std::endl;
        un_socket.close();
    }

}
std::vector<std::string> zmq_coms::explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        remove(token.begin(),token.end(),' ');
        result.push_back(std::move(token));
    }

    return result;
}
std::vector<std::string> zmq_coms::heartbeat()
{

    std::vector<std::string> ret; 
    zmq::message_t reply{};
    
    if(!hb_connected)
    {
        heartbeat_handler();
    }
    else
    {

        zmq::recv_result_t recv_res = hb_socket.recv(reply, zmq::recv_flags::none);

        /*
        * msg structure is
        * device; unique_number; cmd; params;
        * 
        * cmds:
        * 
        * reload 
        * file <complete file_name with path>
        * outstop
        * change_mode <mode>
        * 
        * example:
        * Arduino Micro; file; /conboard/boards/Arduino Micro.json;
        * 
        * Arduino Micro; reload;
        */
        std::string raw = reply.to_string();
        
        std::vector<std::string> parsed_msg = explode(raw,';');
        std::vector<std::string>::iterator msg_it = parsed_msg.begin();
        if(!msg_it->compare(DevName))
        {
            msg_it++;
            if(!msg_it->compare(unique_number))
                {
                    msg_it++;
                    std::vector<std::string> ret(msg_it,parsed_msg.end());
                    return ret;
                }
        }

    } 
    return ret;

}


