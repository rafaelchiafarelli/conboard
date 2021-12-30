#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "version.h"

#include <chrono>
#include <string>
#include <atomic>
#include <vector>


#include "dispatcher.hpp"

#include "crow.h"
#include <unordered_set>
#include <mutex>
using namespace std;


atomic_bool stop;

std::mutex mtx;
std::unordered_set<crow::websocket::connection*> users;
void user_handler(dispatcher *dsp,atomic_bool *lst);

