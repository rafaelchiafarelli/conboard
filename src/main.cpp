#define _GNU_SOURCE
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


static int do_device_list, do_rawmidi_list;
static char *port_name = "default";
static char *send_file_name;
static char *receive_file_name;
static char *send_hex;
static char *send_data;
static int send_data_length;
static int receive_file;
static int dump;
static int timeout;
static int stop;
static snd_rawmidi_t *input, **inputp;
static snd_rawmidi_t *output, **outputp;

static void error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	putc('\n', stderr);
}

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

static void version(void)
{
	puts("amidi version " SND_UTIL_VERSION_STR);
}

static char *my_malloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		error("out of memory");
		exit(EXIT_FAILURE);
	}
	return (char *)p;
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
			error("cannot get rawmidi information %d:%d:%d: %s\n",
			      card, device, sub, snd_strerror(err));
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
		error("cannot open control for card %d: %s", card, snd_strerror(err));
		return;
	}
	device = -1;
	for (;;) {
		if ((err = snd_ctl_rawmidi_next_device(ctl, &device)) < 0) {
			error("cannot determine device number: %s", snd_strerror(err));
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
		error("cannot determine card number: %s", snd_strerror(err));
		return;
	}
	if (card < 0) {
		error("no sound card found");
		return;
	}
	puts("Dir Device    Name");
	do {
		list_card_devices(card);
		if ((err = snd_card_next(&card)) < 0) {
			error("cannot determine card number: %s", snd_strerror(err));
			break;
		}
	} while (card >= 0);
}

static void rawmidi_list(void)
{
	snd_output_t *output;
	snd_config_t *config;
	int err;

	if ((err = snd_config_update()) < 0) {
		error("snd_config_update failed: %s", snd_strerror(err));
		return;
	}
	if ((err = snd_output_stdio_attach(&output, stdout, 0)) < 0) {
		error("snd_output_stdio_attach failed: %s", snd_strerror(err));
		return;
	}
	if (snd_config_search(snd_config, "rawmidi", &config) >= 0) {
		puts("RawMIDI list:");
		snd_config_save(config, output);
	}
	snd_output_close(output);
}

static void load_file(void)
{
	int fd;
	off_t length;

	fd = open(send_file_name, O_RDONLY);
	if (fd == -1) {
		error("cannot open %s - %s", send_file_name, strerror(errno));
		return;
	}
	length = lseek(fd, 0, SEEK_END);
	if (length == (off_t)-1) {
		error("cannot determine length of %s: %s", send_file_name, strerror(errno));
		goto _error;
	}
	send_data = my_malloc(length);
	lseek(fd, 0, SEEK_SET);
	if (read(fd, send_data, length) != length) {
		error("cannot read from %s: %s", send_file_name, strerror(errno));
		goto _error;
	}
	if (length >= 4 && !memcmp(send_data, "MThd", 4)) {
		error("%s is a Standard MIDI File; use aplaymidi to send it", send_file_name);
		goto _error;
	}
	send_data_length = length;
	goto _exit;
_error:
	free(send_data);
	send_data = NULL;
_exit:
	close(fd);
}

static int hex_value(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('A' <= c && c <= 'F')
		return c - 'A' + 10;
	if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	error("invalid character %c", c);
	return -1;
}

static void parse_data(void)
{
	const char *p;
	int i, value;

	send_data = my_malloc(strlen(send_hex)); /* guesstimate */
	i = 0;
	value = -1; /* value is >= 0 when the first hex digit of a byte has been read */
	for (p = send_hex; *p; ++p) {
		int digit;
		if (isspace((unsigned char)*p)) {
			if (value >= 0) {
				send_data[i++] = value;
				value = -1;
			}
			continue;
		}
		digit = hex_value(*p);
		if (digit < 0) {
			send_data = NULL;
			return;
		}
		if (value < 0) {
			value = digit;
		} else {
			send_data[i++] = (value << 4) | digit;
			value = -1;
		}
	}
	if (value >= 0)
		send_data[i++] = value;
	send_data_length = i;
}

/*
 * prints MIDI commands, formatting them nicely
 */
static void print_byte(unsigned char byte)
{
	static enum {
		STATE_UNKNOWN,
		STATE_1PARAM,
		STATE_1PARAM_CONTINUE,
		STATE_2PARAM_1,
		STATE_2PARAM_2,
		STATE_2PARAM_1_CONTINUE,
		STATE_SYSEX
	} state = STATE_UNKNOWN;
	int newline = 0;

	if (byte >= 0xf8)
		newline = 1;
	else if (byte >= 0xf0) {
		newline = 1;
		switch (byte) {
		case 0xf0:
			state = STATE_SYSEX;
			break;
		case 0xf1:
		case 0xf3:
			state = STATE_1PARAM;
			break;
		case 0xf2:
			state = STATE_2PARAM_1;
			break;
		case 0xf4:
		case 0xf5:
		case 0xf6:
			state = STATE_UNKNOWN;
			break;
		case 0xf7:
			newline = state != STATE_SYSEX;
			state = STATE_UNKNOWN;
			break;
		}
	} else if (byte >= 0x80) {
		newline = 1;
		if (byte >= 0xc0 && byte <= 0xdf)
			state = STATE_1PARAM;
		else
			state = STATE_2PARAM_1;
	} else /* b < 0x80 */ {
		int running_status = 0;
		newline = state == STATE_UNKNOWN;
		switch (state) {
		case STATE_1PARAM:
			state = STATE_1PARAM_CONTINUE;
			break;
		case STATE_1PARAM_CONTINUE:
			running_status = 1;
			break;
		case STATE_2PARAM_1:
			state = STATE_2PARAM_2;
			break;
		case STATE_2PARAM_2:
			state = STATE_2PARAM_1_CONTINUE;
			break;
		case STATE_2PARAM_1_CONTINUE:
			running_status = 1;
			state = STATE_2PARAM_2;
			break;
		default:
			break;
		}
		if (running_status)
			fputs("\n  ", stdout);
	}
	printf("%c%02X", newline ? '\n' : ' ', byte);
}

static void sig_handler(int dummy)
{
	stop = 1;
}

static void add_send_hex_data(const char *str)
{
	int length;
	char *s;

	length = (send_hex ? strlen(send_hex) + 1 : 0) + strlen(str) + 1;
	s = my_malloc(length);
	if (send_hex) {
		strcpy(s, send_hex);
		strcat(s, " ");
	} else {
		s[0] = '\0';
	}
	strcat(s, str);
	free(send_hex);
	send_hex = s;
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

    stop = 0;
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
    stop = 0;




    //============================================================================================================================
	int c, err, ok = 0;
	int ignore_active_sensing = 1;
	int do_send_hex = 0;

	while ((c = getopt_long(argc, argv, short_options,
		     		long_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'V':
			version();
			return 0;
		case 'l':
			do_device_list = 1;
			break;
		case 'L':
			do_rawmidi_list = 1;
			break;
		case 'p':
			port_name = optarg;
			break;
		case 's':
			send_file_name = optarg;
			break;
		case 'r':
			receive_file_name = optarg;
			break;
		case 'S':
			do_send_hex = 1;
			if (optarg)
				add_send_hex_data(optarg);
			break;
		case 'd':
			dump = 1;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'a':
			ignore_active_sensing = 0;
			break;
		default:
			error("Try `amidi --help' for more information.");
			return 1;
		}
	}
	if (do_send_hex) {
		/* data for -S can be specified as multiple arguments */
		if (!send_hex && !argv[optind]) {
			error("Please specify some data for --send-hex.");
			return 1;
		}
		for (; argv[optind]; ++optind)
			add_send_hex_data(argv[optind]);
	} else {
		if (argv[optind]) {
			error("%s is not an option.", argv[optind]);
			return 1;
		}
	}

	if (do_rawmidi_list)
		rawmidi_list();
	if (do_device_list)
		device_list();
	if (do_rawmidi_list || do_device_list)
		return 0;

	if (!send_file_name && !receive_file_name && !send_hex && !dump) {
		error("Please specify at least one of --send, --receive, --send-hex, or --dump.");
		return 1;
	}
	if (send_file_name && send_hex) {
		error("--send and --send-hex cannot be specified at the same time.");
		return 1;
	}

	if (send_file_name)
		load_file();
	else if (send_hex)
		parse_data();
	if ((send_file_name || send_hex) && !send_data)
		return 1;

	if (receive_file_name) {
		receive_file = creat(receive_file_name, 0666);
		if (receive_file == -1) {
			error("cannot create %s: %s", receive_file_name, strerror(errno));
			return -1;
		}
	} else {
		receive_file = -1;
	}

	if (receive_file_name || dump)
		inputp = &input;
	else
		inputp = NULL;
	if (send_data)
		outputp = &output;
	else
		outputp = NULL;

	if ((err = snd_rawmidi_open(inputp, outputp, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
		error("cannot open port \"%s\": %s", port_name, snd_strerror(err));
		goto _exit2;
	}

	if (inputp)
		snd_rawmidi_read(input, NULL, 0); /* trigger reading */

	if (send_data) {
		if ((err = snd_rawmidi_nonblock(output, 0)) < 0) {
			error("cannot set blocking mode: %s", snd_strerror(err));
			goto _exit;
		}
		if ((err = snd_rawmidi_write(output, send_data, send_data_length)) < 0) {
			error("cannot send data: %s", snd_strerror(err));
			goto _exit;
		}
	}

	if (inputp) {
		int read = 0;
		int npfds, time = 0;
		struct pollfd *pfds;

		timeout *= 1000;
		npfds = snd_rawmidi_poll_descriptors_count(input);
		pfds = (pollfd *)alloca(npfds * sizeof(struct pollfd));
		snd_rawmidi_poll_descriptors(input, pfds, npfds);
		signal(SIGINT, sig_handler);
		for (;;) {
			unsigned char buf[256];
			int i, length;
			unsigned short revents;

			err = poll(pfds, npfds, 200);
			if (stop || (err < 0 && errno == EINTR))
				break;
			if (err < 0) {
				error("poll failed: %s", strerror(errno));
				break;
			}
			if (err == 0) {
				time += 200;
				if (timeout && time >= timeout)
					break;
				continue;
			}
			if ((err = snd_rawmidi_poll_descriptors_revents(input, pfds, npfds, &revents)) < 0) {
				error("cannot get poll events: %s", snd_strerror(errno));
				break;
			}
			if (revents & (POLLERR | POLLHUP))
				break;
			if (!(revents & POLLIN))
				continue;
			err = snd_rawmidi_read(input, buf, sizeof(buf));
			if (err == -EAGAIN)
				continue;
			if (err < 0) {
				error("cannot read from port \"%s\": %s", port_name, snd_strerror(err));
				break;
			}
			length = 0;
			for (i = 0; i < err; ++i)
				if (!ignore_active_sensing || buf[i] != 0xfe)
					buf[length++] = buf[i];
			if (length == 0)
				continue;
			read += length;
			time = 0;
			if (receive_file != -1)
				write(receive_file, buf, length);
			if (dump) {
				for (i = 0; i < length; ++i)
					print_byte(buf[i]);
				fflush(stdout);
			}
		}
		if (isatty(fileno(stdout)))
			printf("\n%d bytes read\n", read);
	}

	ok = 1;
_exit:
	if (inputp)
		snd_rawmidi_close(input);
	if (outputp)
		snd_rawmidi_close(output);
_exit2:
	if (receive_file != -1)
		close(receive_file);
	return !ok;
}

