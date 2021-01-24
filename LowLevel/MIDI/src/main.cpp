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
 * This is the main module of the system
 *  It is responsible for reading all the devices that are connected to the system - and keep reading it.
 *  launch all threads related to device inputs - one thread for device
 *  kill threads of devices that are no longer connected.
 *  
 *  setup
 *  The system will read the config file and evaluate the existence of the folders json folders (/conboard/json)
 *  the config file also contains:
 *   - the timeout information
 *   - the connection with the external world (keyboard and mouse that will launch events to the PC)
 *   - the maximum number of thread's this code can generate
 *  then, this function can start by openning file descriptors and other tasks related to the output
 *  
 *  start of a thread (dispatcher)
 *  when a device is connected, an event will be called. this event will read information from the device (
 *  such as type, file descriptors, name, input, output, etc) then, this event code, will enqueue a new dispatch
 *  order. The starter will read this information and capture the jsonfile assossiated with the device. 
 *  start a thread of the type of the device inserted with the 
 *  information available.
 *      information required:
 *  - Human Readable name for input and output;
 *  - type (midi, keyboard, mouse or joystick)
 *  
 *  end of a thread (dispatcher)
 *  When a jsonfile changes, or a device is removed, this module mus handle the death of the thread.
 *  First it will send the kill command to the thread than it will "join" with the thread. 
 *  If the event if just a change in the json, a new thread will be launched, however, if it is a device disconnected
 *  then just kill the thread.
 *  
 *  
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

	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
	err = snd_ctl_rawmidi_info(ctl, info);
	if (err >= 0)
		subs_in = snd_rawmidi_info_get_subdevices_count(info);
	else
		subs_in = 0;

	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
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
					    SND_RAWMIDI_STREAM_INPUT :
					    SND_RAWMIDI_STREAM_OUTPUT);
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
		m.devName = string(dev_port);
		hw_ports.push_back(m);
		std::cout<<dev_port<<endl;

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

	stop = true;
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
    cout<<"device list"<<endl;
    device_list();
	cout<<jsonFileName<<endl;
		MIDI *devMIDI;
		
		devMIDI = new MIDI(jsonFileName,hw_ports);

		char cmd[256];
		signal(SIGINT, sig_handler);
		while(!stop)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		devMIDI->Stop();
		delete devMIDI;

    return 0;

}



