#include "zmq_coms.hpp"


zmq_coms::~zmq_coms(){}

zmq_coms::zmq_coms(std::string devName, std::string unique_number_address, std::string coms_address)
{
    coms_socket.connect(coms_address);
    local_connect(unique_number_address);
}

void zmq_coms::local_connect(std::string unique_number_address){
    // connect to the server and get a unique ID
        //send the DevName to the dispatcher and receive a unique number in response
        //there is a catch if two devices with the same name are installed. See dispatcher for more information
        

    st_socket.connect(unique_number_address);


    zmq::message_t request_msg(DevName);
    zmq::send_result_t res_send = st_socket.send(request_msg, zmq::send_flags::none);

    // Get the reply
    zmq::message_t recv_msg;
    zmq::recv_result_t res  = st_socket.recv(recv_msg,zmq::recv_flags::none);
    std::string tmp((char *)recv_msg.data());
    unique_number = tmp;

    st_socket.close();
    
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
    
    //coms_socket.send();
    zmq::recv_result_t recv_res = coms_socket.recv(reply, zmq::recv_flags::none);

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

    return ret;

}
