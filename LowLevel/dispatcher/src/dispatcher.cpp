#include "dispatcher.hpp"
/**
 * Dispatcher works as a buffer or holder for all the communication between the low level
 * and the high level.
 * That means that the lower level sends it's data via zmq_io and the dispatcher sends it 
 * to the user backend if it is connected, if it is not connected, it will not send to any
 * one.
 * On the other hand, if the dispatcher receives a command from the backend from the user,
 * than it will send it to the low level if it is present. 
 * In other terms, the dispatcher ensures that the LowLevel is connected as well as the user
 * Since most communications happen assynchronously, this have to ensure that communications 
 * don misfire or get forgothen.
 * For the vault the ideia is simple. Ensure access to the user, if it is "allowed", and 
 * protect the data in case of access.
 * A data is transfered to the vault only from sharing space and users can tell the system 
 * to protect any given data from the sharing space.
 * But data can only be transfered out of the vault by authentication and decription from 
 * https connection. That means that the size of the vault is unknown, and download can 
 * happen from the internet, only if authorized.
 * It is a known fact that the size of the SDCard is the limiting factor.
 * 
 * The sharing of data is a litle diferent. Data can be only moved around from the USB 
 * connection and have a limited space:
 * - 8mb if the SDCard is smaller than 8G
 * - 25% of the sdcard if the SDCard is 8G
 * - 50% of the sdcard if the SDCard is 16G
 * - 16G for all the other sizes
 * no criptography or protection is allowed/needed
 * 
 */ 

dispatcher::dispatcher()
                    : disp("/conboard/dispatcher/assets/config.json")
                    {
    FileName = "/conboard/dispatcher/assets/config.json";

    
    startup();
}


dispatcher::dispatcher(std::string fileName, 
                        std::atomic_bool *_stop)
                            : disp(fileName)
{
    stop = _stop;
    
    FileName = fileName;
    std::cout<<"dispatcher start"<<std::endl;
    startup();
}

dispatcher::~dispatcher(){
    die();/* kill every thread */

    delete hb;
    delete th_unuique_numb;
    delete io;
    delete http_com;
}

void dispatcher::die(){
    stop = true;
    hb->join();
    th_unuique_numb->join();
    io->join();
    http_com->join();
}

void dispatcher::startup(){
    
    stop = false;

    std::cout<<"start up sequence heart_beat"<<std::endl;
    hb = new std::thread(&dispatcher::th_heart_beat, this);

    std::cout<<"start up sequence unique th_unuique_number"<<std::endl;
    th_unuique_numb = new std::thread(&dispatcher::th_unique_number,this);

    std::cout<<"start up sequence io"<<std::endl;
    io = new std::thread(&dispatcher::th_io, this);
    
    std::cout<<"start up sequence http"<<std::endl;
    http_com = new std::thread(&dispatcher::th_http,this);
}
std::vector<std::string> dispatcher::explode(std::string const & s, char delim)
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
/**
 * This function will receive data from user actions and send it to the upper layers
 * 
 **/ 
void dispatcher::th_io()
{

    io_type io_coms;
    bool parser = disp.GetZMQio(&io_coms);
    io_coms.address.append(":");
    io_coms.address.append(std::to_string(io_coms.port));

    std::cout<<"th_io adr:"<<io_coms.address.c_str()<<std::endl;
    io_socket.bind(io_coms.address);
    
    zmq::message_t message;    
    while(!stop)
    {
        // Waiting for the next request from the client
        //  Block to current statement,  Until the message from the client is received,  Then save it to the message
        
        zmq::recv_result_t res = io_socket.recv(message, zmq::recv_flags::dontwait);
        if(res)
        {
            
            std::string raw = message.to_string();
            
            std::vector<std::string> parsed_msg = explode(raw,';');
            zmq::message_t reply(std::string("OK"));
            zmq::send_result_t res = io_socket.send(reply,zmq::send_flags::dontwait);
            if(!res)
            {
                std::cout<<"Error receiving"<<std::endl;
            }
            std::vector<std::string>::iterator msg_it = parsed_msg.begin();
            
            if(devices.count(*msg_it)>0)
            {
                last_ping[*msg_it] = std::chrono::system_clock::now();//update last ping

            }
            // get the client name and uuid.

            // veryfies that it is in record 

            // if not in record discard the event

            // else update its last_ping status 

            // put the new action into the actions list.

            
            
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    std::cout<<"th_io close"<<std::endl;
    io_socket.close();
}


/**
 * heart beat thread
 *  Will wait for a device send a keep alive/pooling command and response with either 
 *  an "OK" response or a command.
 * 
 * 
 *             
 * // veryfies that it is in record 
 * // if not in record discard the event
 * // else update its last_ping status 
 * // check if there is a command to it and send it back. 
 * // Send OK otherwise
 * //  Send OK to the client information to the client[answer]
 */ 
void dispatcher::th_heart_beat(){

    // thread that is waiting for a connection and receives a solicitation. 
        //a solicitation is a device name that is going 
    std::string resp;
    coms_type coms;
    bool parser = disp.GetZMQcoms(&coms);
    coms.address.append(":");
    coms.address.append(std::to_string(coms.port));

    std::cout<<"th_heart_beat adr:"<<coms.address.c_str()<<"-"<<parser<<std::endl;
    coms_socket.bind(coms.address);
    zmq::message_t message;    
    while(!stop)
    {
        // Waiting for the next request from the client
        //  Block to current statement,  Until the message from the client is received,  Then save it to the message

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

        zmq::recv_result_t res = coms_socket.recv(message, zmq::recv_flags::dontwait);

        if (res)
        {
            resp.clear();
            // get the client name and uuid.
            std::string raw = message.to_string();
            std::vector<std::string> parsed_msg = explode(raw,';');

            if(parsed_msg.size()>1)
            {
                std::vector<std::string>::iterator msg_it = parsed_msg.begin(); //adjust to get the uuid
                
                if(devices.count(*msg_it)>0) //is the uuid present
                {

                    last_ping[*msg_it] = std::chrono::system_clock::now();//update last ping
                    resp = *msg_it;
                    //mutex logic to check the commands queue
                    if(commands.count(*msg_it)>0) //is there a command 
                    {
                        std::string cmd = commands[*msg_it]; 
                        commands.erase(*msg_it); //clear the command
                        resp.append("; ");
                        resp.append(commands[*msg_it]);
                        resp.append(";");
                    }
                    else
                    {
                        resp.append("; OK;");
                    }
                }                
            }
            zmq::message_t reply(resp);
            
            zmq::send_result_t s_res = coms_socket.send(reply,zmq::send_flags::none);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout<<"th_heart_beat close"<<std::endl;
    coms_socket.close();
}


/**
 * unique number thread
 *  Will wait for a device call and answer it with a unique number.
 *  Check the current device map and update if necessáry.
 * 
 */ 
void dispatcher::th_unique_number()
{
    // thread that is waiting for a connection and receives a solicitation. 
        //a solicitation is a device name that is going 
    
    UUID_config uuid;
    disp.GetUUID_cfg(&uuid);

    //char addr[100];
    //memset(addr,0,100);
    //sprintf(addr,"%s:%d",uuid.address.c_str(), uuid.port);
    uuid.address.append(":");
    uuid.address.append(std::to_string(uuid.port));
    std::cout<<"unique number adr:"<<uuid.address.c_str()<<std::endl;
    
    un_socket.bind(uuid.address);
    char data[1024];
    memset(data,1024,0);
    zmq::message_t message((const void *)data,1024);
    while(!stop)
    {
        // Waiting for the next request from the client
        //  Block to current statement,  Until the message from the client is received,  Then save it to the message
        zmq::recv_result_t res = un_socket.recv(message,zmq::recv_flags::dontwait);
        if(res)
        {
            // do some work
            std::string l_devname((char *)message.data());
            std::string l_unique_number = generate_unique_number(l_devname);
            //  Send information to the client[answer]
            
            std::string resp = "";
            resp.append(l_unique_number);            
            resp.append("; ");
            resp.append(l_devname);
            resp.append("; ");
            std::cout<<"th_unique_number-"<<resp<<std::endl;
            zmq::message_t reply(resp);
            zmq::send_result_t s_res = un_socket.send(reply,zmq::send_flags::none);

        }
    
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout<<"th_unique_number close"<<std::endl;
    un_socket.close();
}


std::string dispatcher::generate_unique_number(std::string l_devname)
{
    std::string l_unique_number = "";    
    std::lock_guard<std::mutex> locker(lock_devices);
    uuid_t gen;
    uuid_generate(gen);
    char uuid_unparsed[200];
    uuid_unparse(gen,uuid_unparsed);
    l_unique_number = std::string(uuid_unparsed);
    std::pair<std::string, std::string> n_dev(l_unique_number,l_devname);
    std::pair<std::string, std::chrono::time_point<std::chrono::system_clock>> n_ping(l_unique_number,std::chrono::system_clock::now());
    devices[n_dev.first]=n_dev.second;
    std::cout<<"generate unique number: "<<devices[n_dev.first].c_str()<<std::endl;
    last_ping[n_ping.first] = n_ping.second;
    return l_unique_number;
}

void dispatcher::th_http()
{
    Pistache::Address Addr;
    Http::Endpoint *httpEndpoint;
    httpEndpoint = new Http::Endpoint();
    Rest::Router router;
    std::cout<<"http_start"<<std::endl;

    http_config conf;
    if(disp.GetHTTP(&conf))
    {
        
        auto opts = Http::Endpoint::options()
                        .threads(conf.threads)
                        .maxRequestSize(conf.maxsize)
                        .flags(Pistache::Tcp::Options::ReuseAddr | Pistache::Tcp::Options::FastOpen);
        
        httpEndpoint->init(opts);

        using namespace Rest;
        
        Routes::Post(router, conf.CommandAddr, Routes::bind(&dispatcher::PostCommand, this));
        Routes::Get(router, conf.ConfigAddr, Routes::bind(&dispatcher::GetConfigs, this));
          
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serveThreaded();
        
        while(!stop)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        std::cout<<"http_start close"<<std::endl;
        httpEndpoint->shutdown();
    }
    else
    {
        std::cout<<"http_start ERROR"<<std::endl;
        while(!stop)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void dispatcher::PostCommand(const Rest::Request &req, Http::ResponseWriter writer)
{
    std::cout<<"PostCommand"<<std::endl;
    if (req.method() == Http::Method::Get)
    {
        writer.send(Http::Code::Not_Found);    
    }
    else
    {
        writer.send(Http::Code::Bad_Request);
    }
}

void dispatcher::GetConfigs(const Rest::Request &req, Http::ResponseWriter writer)
{
    std::cout<<"GetConfigs"<<std::endl;
    if (req.method() == Http::Method::Get)
    {
        writer.send(Http::Code::Ok, "The list of all configurations");    
    }
    else
    {
        writer.send(Http::Code::Bad_Request);
    }
}


