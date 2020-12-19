

/*
(C) 2016 Tobias Girstmair, released under the GNU GPL

description: sends a sequence of keystrokes provided from stdin to the hid
device. 
stops typing at: control characters (including newline), chars not in table, EOF
parameters:
	device file (e.g. /dev/hidg0)
	keyboard layout (1=en_us, 2=de_at, 3=de_at-nodeadkeys)
	unicode method: 1=gtk_holddown, 2=gtk_spaceend, 3=windows

	https://oshlab.com/enable-g_serial-usb-otg-console-orange-pi-armbian/
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "keystrokes.hpp"
#include "scancodes.hpp"
#include <string>

using namespace std;
keystrokes::~keystrokes()
{
	fclose (hid_dev);
}
keystrokes::keystrokes(string outputFile, kbdl klayout)
{
	hid_dev = fopen (outputFile.c_str(), "w");
	keyboard_l = klayout;
}

void keystrokes::key_manager(string data, uni_m mode){
		char in_string[TEXT_LEN];

	for (int i = 0; i < strlen (in_string); i++) {

		char tmp[UTF8_MAX_LENGTH] = {in_string[i], in_string[i+1], in_string[i+2], '\0'};
		//TODO: replace by something less stupid
		if (in_string[i] < 128) { // not multi byte
			tmp[1] = '\0';
		} else { // is multi byte
			if (in_string[i] < 0xe0) {
			i++; //skip next thing
			tmp[2] = 0;
			} else {
			i+=2; //WARN: fails on utf8 > 3 byte
			}
		}

		 keysym* s = scan.toscan (tmp);
		if (s == NULL) {
			fprintf (stderr, "Key Symbol not found.\n");
			fclose (hid_dev);
			err = ERR_SYMBOL;
		}
		 layout* l = scan.tolay (s, keyboard_l);
		if (l == NULL) {
			fprintf (stderr, "Unrecognised keyboard layout.\n");
			fclose (hid_dev);
			err = ERR_LAYOUT;
		}
		if (l->key != 0x00) {
			send_key(hid_dev, l->key, l->mod);
			send_key(hid_dev, '\0', '\0'); //release all keys
			if (l->is_dead) {
				//dead keys need to be pressed twice to show up
				send_key(hid_dev, l->key, l->mod);
				send_key(hid_dev, '\0', '\0'); //release all keys
			}
		} else {
			//key does not exist in this layout, use unicode method
			//fprintf (stderr, "Warning: Key '%s'(0x%x) not in this layout!\n", s->sym, s->unicode);
			send_unicode (hid_dev, s->unicode, mode, keyboard_l);
		}
	}
}


void keystrokes::send_key (FILE* hid_dev, unsigned short key, unsigned short mod)
{
	fprintf (hid_dev, "%c%c%c%c%c%c%c%c", mod, '\0', key, '\0', '\0', '\0', '\0', '\0');
}

errors keystrokes::send_unicode (FILE* hid_dev, unsigned int unicode, uni_m method, kbdl klayout)
{
	char buf[10];
	 keysym* s;
	 layout* l;

	if (unicode == 0x00) {
		fprintf (stderr, "Symbol not in lookup table!\n");
		return ERR_SYMBOL;
	}

	switch (method) {
	case SKIP:
		break;
	case GTK_HOLD:
		sprintf (buf, "%x", unicode);
		s = scan.toscan ("u");
		l = scan.tolay (s, klayout);
		send_key (hid_dev, l->key, MOD_LCTRL | MOD_LSHIFT);
		for (int i = 0; i < strlen (buf); i++) {
			s = scan.toscan ((char[2]){buf[i], '\0'});
			l = scan.tolay (s, klayout);
			send_key (hid_dev, l->key, MOD_LCTRL | MOD_LSHIFT);
		}
		send_key (hid_dev, '\0', '\0');
		break;
	case GTK_SPACE:
		sprintf (buf, "%x ", unicode);
		s = scan.toscan ("u");
		l = scan.tolay (s, klayout);
		send_key (hid_dev, l->key, MOD_LCTRL | MOD_LSHIFT);
		for (int i = 0; i < strlen (buf); i++) {
			s = scan.toscan ((char[2]){buf[i], '\0'});
			l = scan.tolay (s, klayout);
			send_key (hid_dev, l->key, MOD_NONE);
		}
		send_key (hid_dev, '\0', '\0');
		break;
	case WINDOWS:
		fprintf (stderr, "windows method not implemented!\n");
		return ERR_LAZY;
	default:
		fprintf (stderr, "unknown unicode method!\n");
		return ERR_LAYOUT; //TODO: better error code
	}
	return ERR_SUCCESS;
}
