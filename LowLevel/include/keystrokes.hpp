#ifndef KEYTHREAD_HPP
#define KEYTHREAD_HPP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_translation_table.hpp"
#include "scancodes.hpp"
#include "pthread.h"
#include "stdio.h"
#include <iostream>
#include <string>
using namespace std;

#define TEXT_LEN 256 //max length of piped text

typedef enum{//argv-indices:
	P_EXE, //executable name
	P_DEV, //device file
	P_LAY, //layout
	P_UNI, //unicode method
	NUM_P  //number of parameters
}params;

typedef enum{//unicode methods:
	SKIP, //ignore any keys not on the layout
	GTK_HOLD, //hold ctrl and shift while entering hex values
	GTK_SPACE, //end hex sequence with spacebar
	WINDOWS //use alt+numpad
}uni_m;

typedef enum{
	ERR_SUCCESS, //no error
	ERR_ARGCOUNT, //wrong number of arguments
	ERR_SYMBOL, //symbol not in look up table
	ERR_LAYOUT, //parameter P_LAY does not contain a correct keyboard layout
	ERR_LAZY //i haven't done this
}errors;

class keystrokes {
    private:
        errors err;
        scancondes scan;
        string o_fName;
        FILE* hid_dev;
        kbdl keyboard_l;
        string data;

    public:
        void send_key (FILE* hid_dev, unsigned short key, unsigned short mod);
        errors send_unicode (FILE* hid_dev, unsigned int unicode, uni_m method, kbdl layout);
        void key_manager(string data, uni_m mode);
        keystrokes(string outputPath, kbdl klayout);
        ~keystrokes(){};

};


#endif