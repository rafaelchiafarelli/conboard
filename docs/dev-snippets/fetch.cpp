

#include "fetch.hpp"

#include <sstream>
#include <string>
#include <fstream>
#include <random>
#define MAX_WORK_TIME 120000

using namespace std;
using namespace Pistache;

fetchGrab::fetchGrab(Address addr, context_t *ctx)
    : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
{
    ctx = ctx;
    srand (time(NULL));
}

void fetchGrab::init(size_t _threads, int _maxRequestSize)
{
    auto opts = Http::Endpoint::options()
                    .threads(_threads)
                    .maxRequestSize(_maxRequestSize)
                    .flags(Pistache::Tcp::Options::ReuseAddr | Pistache::Tcp::Options::FastOpen);
    httpEndpoint->init(opts);
    setupRoutes();
}

void fetchGrab::start()
{
    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serve();
}

void fetchGrab::setupRoutes()
{
    using namespace Rest;
    Routes::Get(router, "/workunified", Routes::bind(&fetchGrab::grabber, this));
    Routes::Get(router, "/work/:id/type", Routes::bind(&fetchGrab::grabberUnified, this));
}

void fetchGrab::grabberUnified(const Rest::Request &req, Http::ResponseWriter writer)
{
    atomic_bool WorkDone;
    auto now = std::chrono::system_clock::now();

    id_picture+=1;
    auto us_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    
    static unsigned int timeout_count[4] = {0,0,0,0};
    int gen_res = 0;
    
    if (req.method() == Http::Method::Get)
    {
        auto id = req.param(":id").as<int>();
        if( id > 3 ) {
            
            writer.send(Http::Code::Bad_Request);
        }
        else
        {
        unsigned int lane = id+1; //stacker picture is 0 for entire image and 1 to 4 to lanes
        WorkDone = false;
        
        auto pic_index = ctx->conf.StartWork((unsigned int ) lane,&WorkDone, now, id_picture);
            if( pic_index == -1 || pic_index > NUMBER_OF_WORK_THREADS)
            {
                writer.send(Http::Code::Not_Found);    
            }
            else
            {
                char FileName[100];
                memset(FileName,0,100);
                sprintf(FileName,"/dev/shm/file",id,pic_index);
                unsigned int WorkTime = 0;

                while(!WorkDone && WorkTime < MAX_WORK_TIME){
                    usleep(10);
                    WorkTime+=1;
                }
                
                WorkDone = 0;
                if(WorkTime >= MAX_WORK_TIME)
                {
                    
                    writer.send(Http::Code::Bad_Request);
                    timeout_count[lane]+=1;
                    if(timeout_count[lane]>OVER_WORK_ERRORS)
                    {
                     exit(ERROR_OVER_WORKED);   
                    }
                }
                else
                {
                    timeout_count[lane] = 0;
                    Http::serveFile(writer,FileName); 
                }
            }
        }
    }
    else
    {
        writer.send(Http::Code::Bad_Request);
    }
}


void fetchGrab::grabber(const Rest::Request &req, Http::ResponseWriter writer)
{
    atomic_bool WorkDone;
    static unsigned int timeout_count;
    auto now = std::chrono::system_clock::now();

    id_picture+=1;
    

    int gen_res = 0;
    if (req.method() == Http::Method::Get)
    {
    WorkDone = false;
    auto pic_index = ctx->conf.StartWork(0,&WorkDone,now, id_picture);
        if( pic_index == -1)
        {
            writer.send(Http::Code::Not_Found);       
        }
        else
        {
            char FileName[40];
            memset(FileName,0,40);
            sprintf(FileName,"/dev/shm/file",pic_index);
            unsigned int WorkTime = 0;

            while(!WorkDone && WorkTime < MAX_WORK_TIME){
                usleep(10);
                WorkTime+=1;
            }
            
            WorkDone = 0;
            if(WorkTime >= MAX_WORK_TIME)
            {
                
                writer.send(Http::Code::Bad_Request);
                timeout_count+=1;
                if(timeout_count>OVER_WORK_ERRORS)
                {
                    exit(ERROR_OVER_WORKED);   
                }
            }
            else
            {
                timeout_count = 0;
                Http::serveFile(writer,FileName); 
            }
        }
    }
}
