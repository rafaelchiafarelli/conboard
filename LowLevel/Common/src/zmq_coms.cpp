#include "zmq_coms.hpp"
#include "envelope_version.hpp"
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

zmq_coms::~zmq_coms(){
    die();
    delete io_thread;
}

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
    stop = false;
    io_thread = new std::thread(&zmq_coms::th_io,this);
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
    std::cout<<"th io start"<<std::endl;
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
            // request
            /**
             *  UUID; DevName; action;
             * 
             * 
            */
            // INTERFACE.md O2: envelope version trails the frame (NOT leading --
            // see envelope_version.hpp for why: explode()'s `remove(...' ')`
            // without a follow-up `erase()` leaves garbage on any token that had
            // a leading space, i.e. every token except field 0. Field 0 must stay
            // the uuid, so the version goes last instead).
            std::string msg=unique_number;
            msg.append("; ");
            msg.append(DevName);
            msg.append("; ");
            msg.append(to_send);
            msg.append("; ");
            msg.append(CONBOARD_ENVELOPE_VERSION);
            msg.append(";");
            zmq::message_t request_msg(msg);
            
            zmq::send_result_t res_send = io_socket.send(request_msg, zmq::send_flags::dontwait);
            // Get the reply
            std::string resp = "";
            if(res_send)
            {
                std::cout<<"IOwill wait"<<std::endl;
                zmq::message_t recv_msg;
                zmq::recv_result_t res  = io_socket.recv(recv_msg,zmq::recv_flags::none);
                if(!res)
                {
                    // No reply within RCV_TIMEOUT_MS (dispatcher unreachable/down).
                    // A REQ socket that timed out mid-reply can't just send() again
                    // -- it's still "owed" a recv -- so reconnect on a fresh socket
                    // right away (the `!io_connected` / WAIT_FOR_TRIES path below is
                    // a much slower fallback, meant for "never got connected" at
                    // startup, not a mid-flight timeout).
                    io_socket.close();
                    io_socket = zmq::socket_t(io_context, zmq::socket_type::req);
                    io_handler();
                }
            }
        }
        else if (!io_connected)
        {
            tries+=1;
            if(tries > WAIT_FOR_TRIES){
                io_handler();
                tries = 0;
            }
        }
        if(!send_io)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
bool zmq_coms::dispatch(std::string msg)
{
    // INTERFACE.md O4: reporting path only (HID output is unaffected -- see
    // DeviceEngine::oQueue). On overflow, evict the OLDEST queued report rather
    // than reject the incoming one: a live monitor cares about current state,
    // not a complete backlog of stale events, so this keeps the stream from
    // lagging further behind reality during a sustained burst. Return value
    // still signals "something was dropped" so callers can log it.
    bool ret = true;
    {
        std::lock_guard<std::mutex> locker(io_mu);
        if(io_queue.size() >= STACKED_IO_MSG)
        {
            io_queue.pop();
            ret = false;
        }
        io_queue.push(msg);
    }
    return ret;
}
/**
 *  Connect to the io server
 * 
 **/ 
void zmq_coms::io_handler()
{
    io_socket.setsockopt(ZMQ_LINGER, 10);
    io_socket.setsockopt(ZMQ_RCVTIMEO, RCV_TIMEOUT_MS);
    io_connected = io_socket.connect(io_address);
    if(!io_connected)
        std::cout<<"IO server not connected"<<std::endl;
}
/**
 * connect to the heartbeat server
 **/ 
void zmq_coms::heartbeat_handler()
{
    hb_socket.setsockopt(ZMQ_LINGER, 10);
    hb_socket.setsockopt(ZMQ_RCVTIMEO, RCV_TIMEOUT_MS);
    hb_connected = hb_socket.connect(hb_address);
    if(!hb_connected)
        std::cout<<"Heartbeat not connected to:"<<hb_address.c_str()<<std::endl;
}
/**
 * connect to the unique number server
 **/ 
void zmq_coms::unique_number_handler(){
    // connect to the server and get a unique ID
        //send the DevName to the dispatcher and receive a unique number in response
        //there is a catch if two devices with the same name are installed. See dispatcher for more information
    un_socket.setsockopt(ZMQ_LINGER, 10);
    un_socket.setsockopt(ZMQ_RCVTIMEO, RCV_TIMEOUT_MS);
    un_connected = un_socket.connect(un_address);
    if(un_connected)
    {
        zmq::message_t request_msg(DevName);
        zmq::send_result_t res_send = un_socket.send(request_msg, zmq::send_flags::dontwait);
        // Get the reply
        if(res_send)
        {
            zmq::message_t recv_msg;
            zmq::recv_result_t res  = un_socket.recv(recv_msg,zmq::recv_flags::none);
            if(res)
            {
                std::string raw = recv_msg.to_string();
                std::vector<std::string> parsed_msg = explode(raw,';');
                if(parsed_msg.size() > 1)
                {
                    unique_number = *(parsed_msg.begin());
                }
            }
            std::cout<<"DevName:"<<DevName<<" unique number:"<<unique_number<<std::endl;
        }
    }
}
std::vector<std::string> zmq_coms::explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);
    for (std::string token; std::getline(iss, token, delim); )
    {
        // std::remove compacts non-space chars to the front and returns the new
        // logical end -- it does NOT shrink the string. Without erase() here,
        // every token after the first (i.e. every one that had a leading space
        // from the "; " separator) keeps its original length with a leftover
        // duplicated trailing char (e.g. "OK" -> "OKK"), breaking any exact
        // string compare against it (found while verifying O2: this is why
        // heartbeat-delivered reload/file/outstop commands never fired).
        token.erase(remove(token.begin(),token.end(),' '), token.end());
        result.push_back(std::move(token));
    }
    return result;
}
std::vector<std::string> zmq_coms::heartbeat()
{
    std::vector<std::string> ret; 
    if(!hb_connected)
    {
        heartbeat_handler();
    }
    else
    {
        /*
        * msg structure is
        * USER SEND
        * device; unique_number;
        * 
        * USER RECEIVES 
        * unique_number; OK;
        * or 
        * unique_number; cmd; params;
        * 
        * where cmds are:
        * unique_number; reload; 
        * unique_number; file; <complete file_name with path>;
        * unique_number; outstop;
        * unique_number; change_mode; <mode>;
        * 
        * example:
        * USER SEND
        * Arduino Micro; 4503:2342:2342:2342:2342;
        * 4503:2342:2342:2342:2342; file; /conboard/boards/Arduino Micro.json;
        * 
        * example2:
        * USER SEND
        * Arduino Micro; 4503:2342:2342:2342:2342;
        * 4503:2342:2342:2342:2342; reload;
        * 
        * example3:
        * USER SEND
        * Arduino Micro; 4503:2342:2342:2342:2342;
        * 4503:2342:2342:2342:2342; outstop;
        * 
        */
        
        // INTERFACE.md O2: envelope version trails the frame (see th_io() for
        // why -- explode()'s remove()-without-erase() quirk corrupts any token
        // that isn't field 0).
        std::string msg=unique_number;
        msg.append("; ");
        msg.append(DevName);
        msg.append("; ");
        msg.append(CONBOARD_ENVELOPE_VERSION);
        msg.append(";");
        zmq::message_t req_message(msg);
        zmq::send_result_t send_res = hb_socket.send(req_message, zmq::send_flags::dontwait);
        if(send_res)
        {
            zmq::message_t reply;
            std::cout<<"heartbeat"<<std::endl;
            zmq::recv_result_t res = hb_socket.recv( reply, zmq::recv_flags::none);
            if(res)
            {
                std::string raw = reply.to_string();
                std::vector<std::string> parsed_msg = explode(raw,';');
                // Reply carries a trailing version token too (dispatcher side);
                // nothing below reads past the fields it already expects, so it's
                // ignored here rather than stripped.
                if(parsed_msg.size()>1) //check if the message is making sense
                {
                    std::vector<std::string>::iterator msg_it = parsed_msg.begin();
                    if(!unique_number.compare(*msg_it)) //filter messages to this device
                    {
                        ret = parsed_msg;
                    }
                }
            }
            else
            {
                // No reply within RCV_TIMEOUT_MS (e.g. the dispatcher was stopped
                // out from under us -- install-on-device.sh stops it before the
                // per-device handlers). A REQ socket that timed out mid-reply can't
                // send() again -- it's still "owed" a recv -- so reconnect on a
                // fresh socket; heartbeat_handler() runs again on the next call
                // since hb_connected is now false. Without this, coms_handler()'s
                // thread blocks here forever and stopEngine() can never join it.
                hb_socket.close();
                hb_socket = zmq::socket_t(hb_context, zmq::socket_type::req);
                hb_connected = false;
            }
        }
    }
    return ret;
}


