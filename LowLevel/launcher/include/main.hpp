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

#include <iostream>
#include <chrono>
#include <string>
#include <atomic>
#include <vector>
using namespace std;

void read_all(char *devInfo, char *path);
void create_xml(char *devInfo, char *folder);
void clear(char *folder);

atomic_bool stop;

