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
#include <alsa/asoundlib.h>
#include "aconfig.h"
#include "version.h"

#include "main.hpp"

#include "midithread.hpp"
#include <chrono>
#include <string>
#include <atomic>
#include <vector>


using namespace std;



/**
 * This is the TouchScreen module of the system, and it is launched by the launcher application
 *  It is responsible for reading all the TouchScreen devices that are connected to the system,
 *  launch the correct handler and communitate with the backend to adapt to changes in the json.
 *  
 *  setup
 *  The launcher selects a json and, inside the json, there is information pertinent to this device (macros and adresses), information
 *  to comunicate with the backend and driver specific information
 *  
 *  start of a thread
 *  The starter will read the information available in the json file passed thru command line and capture the 
 *  events of this device. 
 *  information available.
 *      information required:
 *  - Human Readable name for input and output;
 *  - type (midi, keyboard, mouse or joystick)
 *  
 *  end of a device
 *  When a device is removed, there is no event detectable, so, the application has to handle this event by stoping it self.
 *  If the event if just a change in the json, a new thread will be launched, however, if it is a device disconnected
 *  then just kill the thread.
 *  
 *  change the json
 *  user can change macros and/or specifics of the device.
 *  The j
 * 
 * 
 **/ 


static void list_device(snd_ctl_t *ctl, int card, int device)
{
	snd_rawmidi_info_t *info;
	const char *name;
	const char *sub_name;
	int subs, subs_in, subs_out;
	int sub;
	int err;
	
	snd_rawmidi_info_alloca(&info);
	snd_rawmidi_info_set_device(info, device);

	snd_rawmidi_info_set_stream(info, SND_RAWTouchScreen_STREAM_INPUT);
	err = snd_ctl_rawmidi_info(ctl, info);
	if (err >= 0)
		subs_in = snd_rawmidi_info_get_subdevices_count(info);
	else
		subs_in = 0;

	snd_rawmidi_info_set_stream(info, SND_RAWTouchScreen_STREAM_OUTPUT);
	err = snd_ctl_rawmidi_info(ctl, info);
	if (err >= 0)
		subs_out = snd_rawmidi_info_get_subdevices_count(info);
	else
		subs_out = 0;

	subs = subs_in > subs_out ? subs_in : subs_out;
	if (!subs){
		std::cout<<"Device Error IO"<<endl;
		return;
	}
	
	std::cout<<"list_device size:"<<subs<<endl;


	for (sub = 0; sub < subs; ++sub) {
		raw_midi m;
		m.card = card;
		m.device = device;
		m.sub = sub;
		snd_rawmidi_info_set_stream(info, sub < subs_in ?
					    SND_RAWTouchScreen_STREAM_INPUT :
					    SND_RAWTouchScreen_STREAM_OUTPUT);
		snd_rawmidi_info_set_subdevice(info, sub);
		err = snd_ctl_rawmidi_info(ctl, info);
		if (err < 0) {
 
			return;
		}
		m.name = string(snd_rawmidi_info_get_name(info));
		
		sub_name = snd_rawmidi_info_get_subdevice_name(info);
		m.sub_name = string(snd_rawmidi_info_get_subdevice_name(info));
		char dev_port[256];
		if (sub == 0 && sub_name[0] == '\0') {
			
			sprintf(dev_port,"hw:%d,%d,%d",
			       card, device,sub);
			break;
		} else {
			sprintf(dev_port,"hw:%d,%d,%d",
			       card, device, sub);
		}
		m.port = string(dev_port);
		std::cout<<m<<endl;
		hw_ports.push_back(m);
		

 	}
}

static void list_card_devices(int card)
{
	snd_ctl_t *ctl;
	char name[32];
	int device;
	int err;
	sprintf(name, "hw:%d", card);
	if ((err = snd_ctl_open(&ctl, name, 0)) < 0) {
		return;
	}

	device = -1;
	for (;;) {

		if ((err = snd_ctl_rawmidi_next_device(ctl, &device)) < 0) {
			break;
		}
		if (device < 0)
			break;
		list_device(ctl, card, device);
	}
	if(err<0)
		std::cout<<"Error: "<<err<<endl;
	err = snd_ctl_close(ctl);
	if(err<0)
		std::cout<<"Error closing: "<<err<<endl;
}

static void device_list(void)
{
	int card, err;

	card = -1;
	if ((err = snd_card_next(&card)) < 0) {
		std::cout<<"No device found"<<endl;
		return;
	}
	if (card < 0) {
	
		return;
	}
		std::cout<<"Dir Device    Name"<<endl;
	do {
		list_card_devices(card);
		if ((err = snd_card_next(&card)) < 0) {
		
			break;
		}
	} while (card >= 0);
}

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
		cout<<"error, must specifi json. Usage ./midi -x \"/home/user/file.json\""<<endl;
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
    device_list();

	TouchScreen *devTouchScreen;
	
	devTouchScreen = new TouchScreen(jsonFileName,hw_ports);

	signal(SIGINT,  sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGQUIT, sig_handler);

	while(!stop)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//messenger must be here
	}

	devTouchScreen->Stop();
	delete devTouchScreen;

    return 0;

}



