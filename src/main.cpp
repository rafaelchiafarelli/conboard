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



#include "midithread.hpp"
#include <chrono>
#include <string>
#include <atomic>


using namespace std;

atomic_bool stop;
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

static void usage(void)
{
	printf(
		"Usage: amidi options\n"
		"\n"
		"-h, --help             this help\n"
		"-V, --version          print current version\n"
		"-l, --list-devices     list all hardware ports\n"
		"-L, --list-rawmidis    list all RawMIDI definitions\n"
		"-p, --port=name        select port by name\n"
		"-s, --send=file        send the contents of a (.syx) file\n"
		"-r, --receive=file     write received data into a file\n"
		"-S, --send-hex=\"...\"   send hexadecimal bytes\n"
		"-d, --dump             print received data as hexadecimal bytes\n"
		"-t, --timeout=seconds  exits when no data has been received\n"
		"                       for the specified duration\n"
		"-a, --active-sensing   don't ignore active sensing bytes\n");
}


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
	if (!subs)
		return;

	for (sub = 0; sub < subs; ++sub) {
		snd_rawmidi_info_set_stream(info, sub < subs_in ?
					    SND_RAWMIDI_STREAM_INPUT :
					    SND_RAWMIDI_STREAM_OUTPUT);
		snd_rawmidi_info_set_subdevice(info, sub);
		err = snd_ctl_rawmidi_info(ctl, info);
		if (err < 0) {

			return;
		}
		name = snd_rawmidi_info_get_name(info);
		sub_name = snd_rawmidi_info_get_subdevice_name(info);
		if (sub == 0 && sub_name[0] == '\0') {
			printf("%c%c  hw:%d,%d    %s",
			       sub < subs_in ? 'I' : ' ',
			       sub < subs_out ? 'O' : ' ',
			       card, device, name);
			if (subs > 1)
				printf(" (%d subdevices)", subs);
			putchar('\n');
			break;
		} else {
			printf("%c%c  hw:%d,%d,%d  %s\n",
			       sub < subs_in ? 'I' : ' ',
			       sub < subs_out ? 'O' : ' ',
			       card, device, sub, sub_name);
		}
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
	puts("Dir Device    Name");
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
	static const char short_options[] = "hVlLp:s:r:S::dt:a";
	static const struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"version", 0, NULL, 'V'},
		{"list-devices", 0, NULL, 'l'},
		{"list-rawmidis", 0, NULL, 'L'},
		{"port", 1, NULL, 'p'},
		{"send", 1, NULL, 's'},
		{"receive", 1, NULL, 'r'},
		{"send-hex", 2, NULL, 'S'},
		{"dump", 0, NULL, 'd'},
		{"timeout", 1, NULL, 't'},
		{"active-sensing", 0, NULL, 'a'},
		{ }
	};
    //============================================================================================================================

    stop = false;
    device_list();
    char p_name[] = {"hw:1,0,0"};
    string xmlFileName("/home/pi/conboard/MIDI_DJTech4Mix.xml");
    MIDI testMIDI(p_name, xmlFileName);
	signal(SIGINT, sig_handler);
    while(!stop)
    {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    testMIDI.Stop();

   
    return 0;
}

