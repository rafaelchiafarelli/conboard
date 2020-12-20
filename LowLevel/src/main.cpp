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
 *  The system will read the config file and evaluate the existence of the folders xml folders (/conboard/xml)
 *  the config file also contains:
 *   - the timeout information
 *   - the connection with the external world (keyboard and mouse that will launch events to the PC)
 *   - the maximum number of thread's this code can generate
 *  then, this function can start by openning file descriptors and other tasks related to the output
 *  
 *  start of a thread (dispatcher)
 *  when a device is connected, an event will be called. this event will read information from the device (
 *  such as type, file descriptors, name, input, output, etc) then, this event code, will enqueue a new dispatch
 *  order. The starter will read this information and capture the xmlfile assossiated with the device. 
 *  start a thread of the type of the device inserted with the 
 *  information available.
 *      information required:
 *  - Human Readable name for input and output;
 *  - type (midi, keyboard, mouse or joystick)
 *  - xmlfile and xsdfile
 *  
 *  end of a thread (dispatcher)
 *  When a xmlfile changes, or a device is removed, this module mus handle the death of the thread.
 *  First it will send the kill command to the thread than it will "join" with the thread. 
 *  If the event if just a change in the xml, a new thread will be launched, however, if it is a device disconnected
 *  then just kill the thread.
 *  
 *  
 * 
 * 
 **/ 


atomic_bool stop;
vector<raw_midi> hw_ports;


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
		m.sub_name = string(snd_rawmidi_info_get_subdevice_name(info));
		char dev_port[256];
		if (sub == 0 && sub_name[0] == '\0') {
			
			sprintf(dev_port,"%c%c  hw:%d,%d    %s",
			       sub < subs_in ? 'I' : ' ',
			       sub < subs_out ? 'O' : ' ',
			       card, device, name);
			
			if (subs > 1)
				printf(" (%d subdevices)", subs);
			putchar('\n');
			break;
		} else {
			sprintf(dev_port,"%c%c  hw:%d,%d,%d  %s %s",
			       sub < subs_in ? 'I' : ' ',
			       sub < subs_out ? 'O' : ' ',
			       card, device, sub, sub_name,name);
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
	snd_ctl_close(ctl);
}

static void device_list(void)
{
	int card, err;

	card = -1;
	if ((err = snd_card_next(&card)) < 0) {
		
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
	static const char short_options[] = "i:p:x";
	static const struct option long_options[] = {
		{"ID", 1, NULL, 'i'},
		{"port", 1, NULL, 'p'},
		{"xml", 1, NULL, 'x'},
		{ }
	};
    
	if(argc < 4)
		{
			std::cout<<"error, must specifi port and xml. Usage ./midi -p: \"hw:1,0,0\" -x \"/home/user/file.xml\""<<endl;
			return -1;
		}
    char p_name[256];
	char fifoFile[256];
    string xmlFileName;
	int c;
	while ((c = getopt_long(argc, argv, short_options,
		     		long_options, NULL)) != -1) {
		switch (c) {

		case 'p':
			sprintf(p_name,"%s",optarg);
			break;
		case 'x':
			xmlFileName = string(optarg);
			break;
		case 'd':
			sprintf(fifoFile, "%s",optarg);
			break;

		default:
			std::cout<<"Try more information."<<endl;
			return 1;
		}
	}
    
    device_list();
	/*
		char p_name[] = {"hw:1,0,0"};
		string xmlFileName("/home/pi/conboard/MIDI_DJTech4Mix.xml");
	*/
	std::cout<<xmlFileName<<endl;
	if(!hw_ports.empty())
	{
		for(vector<raw_midi>::iterator it = hw_ports.begin();
			it!=hw_ports.end();
			it++)
		{
			if(!it->port.compare(p_name))
			{
				stop = false;
				std::cout<<"Found a compatible port"<<endl;
				break;
			}
		}
	}
	else
	{
		stop = true;
	}
	if(!stop)
	{
		MIDI *devMIDI;
		int fd = 0;
		fd = open(fifoFile,O_RDONLY| O_NONBLOCK); 
		if(fd < 0)
		{
			std::cout<<"error opening named pipe (fifo)"<<endl;
			return -1;
		}
		devMIDI = new MIDI(p_name, xmlFileName);
		mkfifo(fifoFile, 0666); 
		char cmd[80];
		signal(SIGINT, sig_handler);
		while(!stop)
		{
			//listen to a fifo for hi level commands
			/**
			 * Commands: 
			 * 		- reload: will stop the thread, reload the xml file and re-start the thread
			 * 		- file: send midi cmds to a file
			 * 		- outstop: stop sending cmds to file.
			 */ 
			
			int err = read(fd, cmd, 80); 
			if(err<0)
			{
				std::cout<<"Fifo closed"<<endl;
				stop = true;
			}
			else
			{
				// cmd param1 param2 param3....
				string raw = string(cmd);
				string command = raw.substr(0,raw.find(' '));
				if(!command.compare("reload"))
				{
					std::cout<<"Reload!"<<endl;
					devMIDI->Reload();
				}
				else if(!command.compare("file"))
				{
					string param = raw.substr(raw.find(' '), raw.size());
;					devMIDI->outFile(param);
				}
				else if(!command.compare("outstop"))
				{
					devMIDI->outStop();
				}
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if(fd)
			close(fd); 
		devMIDI->Stop();
		delete devMIDI;
	}
	else
	{
		std::cout<<"File not found"<<endl;
	}
    return 0;
}


