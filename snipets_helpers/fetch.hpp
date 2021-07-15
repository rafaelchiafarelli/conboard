#ifndef FETCHGEAB_HPP
#define FETCHGEAB_HPP
#include "main.h"
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>

using namespace std;
using namespace Pistache;
#define OVER_WORK_ERRORS 200
class fetchGrab
{   
    public:
    atomic_int32_t id_picture;

    private:
    void setupRoutes();
    void grabberUnified(const Rest::Request &request, Http::ResponseWriter response);
    void grabber(const Rest::Request &request, Http::ResponseWriter response);
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router                    router;
    context_t *Main_ctx;
    public:
    
    fetchGrab(Address add, context_t *ctx);
    void init(size_t _threads = 10, int _maxRequestSize = 500 * 1024 * 1024);
    void start();
};

#endif
