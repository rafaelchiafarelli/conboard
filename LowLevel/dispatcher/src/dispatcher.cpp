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
                        Pistache::Address addr, 
                        std::atomic_bool *_stop)
                            : disp(fileName)
{
    stop = _stop;
    
    FileName = fileName;
    std::cout<<"dispatcher start"<<std::endl;
    startup();
}

dispatcher::~dispatcher(){

}

void dispatcher::die(){
    stop = true;
    for (std::thread & th : vth)
    {
        // If thread Object is Joinable then Join that thread.
        th.join();
    }
}

void dispatcher::startup(){
    //unique_number thread
    stop = false;
    std::cout<<"start up sequence"<<std::endl;
    std::thread hb(&dispatcher::th_heart_beat, this);
    vth.push_back(std::move(hb));
    std::thread numb(&dispatcher::th_unique_number,this);
    vth.push_back(std::move(numb));
    std::thread http_com(&dispatcher::http_start,this);
    vth.push_back(std::move(http_com));
}

/**
 * heart beat thread
 *  Will wait for a device send a keep alive/pooling command and response with either 
 *  an "OK" response or a command.
 * 
 * 
 * 
 */ 
void dispatcher::th_heart_beat(){

    // thread that is waiting for a connection and receives a solicitation. 
        //a solicitation is a device name that is going 
    std::cout<<"th_heart_beat"<<std::endl;
    zmq_coms coms;
    disp.GetZMQcoms(&coms);

    char addr[100];
    memset(addr,0,100);
    sprintf(addr,"%s:%d",coms.address, coms.port);

    coms_socket.bind(addr);
    
    while(!stop)
    {
        zmq::message_t message;

        // Waiting for the next request from the client
        //  Block to current statement,  Until the message from the client is received,  Then save it to the message
        bool res = coms_socket.recv(&message, ZMQ_DONTWAIT);
        if(res)
        {
            //  Send OK to the client information to the client[answer]
            char resp[100];
            sprintf(resp, "OK");
            zmq::message_t reply(std::strlen(resp));
            memcpy(reply.data(), resp, std::strlen(resp));
            coms_socket.send(reply,ZMQ_DONTWAIT);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    std::cout<<"th_heart_beat close"<<std::endl;
    coms_socket.close();

}


/**
 * unique number thread
 *  Will wait for a device call and answer it with a unique number.
 *  Check the current device map and update if necessáry.
 *  
 * 
 * 
 */ 
void dispatcher::th_unique_number()
{
    // thread that is waiting for a connection and receives a solicitation. 
        //a solicitation is a device name that is going 
    std::cout<<"th_unique_number"<<std::endl;
    UUID_config uuid;
    disp.GetUUID_cfg(&uuid);

    char addr[100];
    memset(addr,0,100);
    sprintf(addr,"%s:%d",uuid.address, uuid.port);
    st_socket.bind(addr);

    std::cout<<"th_unique_number binded"<<std::endl;
    while(!stop)
    {
        zmq::message_t message;

        // Waiting for the next request from the client
        //  Block to current statement,  Until the message from the client is received,  Then save it to the message
        bool res = st_socket.recv(&message, ZMQ_DONTWAIT);
        //.recv(&message,zmq::recv_flags::dontwait);
        if(res)
        {
            // do some work
            std::string l_devname((char *)message.data());
            std::string l_unique_number = generate_unique_number(l_devname);

            //  Send information to the client[answer]
            char resp[100];
            sprintf(resp, "%s %s",l_devname,l_unique_number);
            zmq::message_t reply(std::strlen(resp));
            memcpy(reply.data(), resp, std::strlen(resp));
            st_socket.send(reply,ZMQ_DONTWAIT);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    std::cout<<"th_unique_number close"<<std::endl;
    st_socket.close();
}


std::string dispatcher::generate_unique_number(std::string l_devname)
{
    std::string l_unique_number = "";    
    std::lock_guard<std::mutex> locker(lock_devices);
    uuid_t gen;
    uuid_generate(gen);
    l_unique_number = std::string((char *)gen);
    std::map<std::string, std::string>::iterator dev = devices.find(l_devname);
    std::pair<std::string, std::string> n_dev(l_devname, l_unique_number);

    if(dev->second.empty())
    {
        devices.insert(n_dev);
    }
    else
    {
        //if there is a devname already, generate a new one and send it back to the receiver
        devices.erase(l_devname); //delete the one in here
        //add to the map
        devices.insert(n_dev);
    }
    return l_unique_number;
}

void dispatcher::http_start()
{
    Pistache::Address Addr;
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    
    Rest::Router router;
    std::cout<<"http_start"<<std::endl;

    http_config conf;
    if(disp.GetHTTP(&conf))
    {
        auto opts = Http::Endpoint::options()
                        .threads(conf.port)
                        .maxRequestSize(conf.threads)
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


