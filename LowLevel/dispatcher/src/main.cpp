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

#include "main.hpp"

#include <chrono>
#include <string>
#include <atomic>
#include <vector>


using namespace std;



/**
 * This is the dispatcher for the system
 * All devices launched will connect to this dispatcher and receive a unique handler. 
 * This handler will be used to talk to the BackEnd application. They will send their users interactions
 * and will receive commands:
 * - Update-json: {"update":<file name>}
 * - Restart device: {"restart":<level of restart>}
 * - change mode of operation: {"change_mode":<number of mode of operation>}
 * - 
 * - 
 * Extra features:
 * Handle the vault (with encription Key)
 *  - Add files
 *  - Remove files
 *  - Change encription key
 *  - verify integrity
 *  - 
 * Handle the files shared
 *  - add files to the img
 *  - remove files from the img
 * 
 * Handle the communication with the external screen 
 *  - put the current IP address on the screen
 *  - put the secret code to log in on the screen
 *  
 **/ 


static void sig_handler(int dummy)
{
	stop = true;
}

int main(int argc, char *argv[])
{

	stop = false;
	static const char short_options[] = "x:";
	static const struct option long_options[] = {
		{"json", 1, NULL, 'x'},
		{ }
	};
    
	if(argc < 2)
	{
		cout<<"error, must specifi json. Usage ./dispatcher -x \"/home/user/file.json\""<<endl;
		return -1;
	}
    string jsonFileName;
	int c;
	while ((c = getopt_long(argc, 
							argv, 
							short_options,
				     		long_options, NULL)) != -1) 
	{
		switch (c) {
		case 'x':
			jsonFileName = string(optarg);
			break;
		default:
			cout<<"Try more information."<<endl;
			return 1;
		}	
	}

	signal(SIGINT,  sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGQUIT, sig_handler);

	while(!stop)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//messenger must be here
	}



    return 0;

}



