#pragma once
#include <iostream>
#include <atomic>
#include "actions.h"
#include "keyNumbers.hpp"
#define BUF_LEN 512
using namespace std;



struct options {
	const char    *opt;
	unsigned char val;
};
static struct options kmod[] = {
    {.opt = "--left-ctrl",		.val = 0x01},
    {.opt = "--right-ctrl",		.val = 0x10},
    {.opt = "--left-shift",		.val = 0x02},
    {.opt = "--right-shift",	.val = 0x20},
    {.opt = "--left-alt",		.val = 0x04},
    {.opt = "--right-alt",		.val = 0x40},
    {.opt = "--left-meta",		.val = 0x08},
    {.opt = "--right-meta",		.val = 0x80},
    {.opt = NULL}
};


static struct options mmod[] = {
    {.opt = "--b1", .val = 0x01},
    {.opt = "--b2", .val = 0x02},
    {.opt = "--b3", .val = 0x04},
    {.opt = NULL}
};

static struct options jmod[] = {
    {.opt = "--b1",		.val = 0x10},
    {.opt = "--b2",		.val = 0x20},
    {.opt = "--b3",		.val = 0x40},
    {.opt = "--b4",		.val = 0x80},
    {.opt = "--hat1",	.val = 0x00},
    {.opt = "--hat2",	.val = 0x01},
    {.opt = "--hat3",	.val = 0x02},
    {.opt = "--hat4",	.val = 0x03},
    {.opt = "--hatneutral",	.val = 0x04},
    {.opt = NULL}
};

class oActions{
    private:
        int fd = 0;
        char const *hid_name = {"/dev/ghid0"};    
        atomic_bool stop;


    public:
        int joystick_fill_report(char report[8], char buf[BUF_LEN], bool *hold);
        int keyboard_send(keyType type, char *buf, size_t length, bool *hold);
        int mouse_fill_report(char report[8], char buf[BUF_LEN], bool *hold);
        int send_out(char *buf,size_t size, bool hold);
        int out_thred();        
        oActions(char *devName){};
        ~oActions(){};
        virtual void oMouse(mouseActions){};
        virtual void oJoystick(joystickActions){};
        virtual void oKeyboard(keyboardActions){};

};