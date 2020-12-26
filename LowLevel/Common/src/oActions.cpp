
#include "oActions.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <string>

#include <iomanip>

#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <unistd.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

#include <keyNumbers.hpp>

using namespace std;

oActions::oActions(char *devName)
{

	fd = open(hid_name, O_RDWR, 0666);
	cout<<"file: "<<hid_name<<" fd:"<<fd<<endl;

}

int oActions::send_out(char *buf,size_t size, bool h)
{
    char report[8];
    bool hold = h;
    memset(report, 0x0, sizeof(report));
    keyType dummy;
    int to_send = keyboard_send(dummy, buf,size, &hold);

    //int to_send = mouse_fill_report(report, buf, &hold);

    //int to_send = joystick_fill_report(report, buf, &hold);
return 0;
}

int oActions::keyboard_send(keyType type, char *buf, size_t length, bool *hold)
{
	int key = 0;
	int i = 0;
    char report[8];
    memset(report,0,8);
	size_t to_send;
	cout<<"keyboard_send"<<endl;	
	switch(type){
		case hotkey:
			// do stuff
		break;
		case text:
		break;
		case oneKey:
			cout<<"buf:"<<buf<<" comprimento:"<<length<<endl;
			namedKeyCodes key=in_word_set(buf,length);
			cout<<"key->name"<<key.name<<"key->number"<<key.number<<endl;
			report[3] = key.number;
			to_send = 8;
			cout<<"file desc:"<<fd<<endl;
			int sent_data = write(fd, report, to_send);
			cout<<"data sent:"<<sent_data<<endl;
			if (!hold) {
				memset(report, 0x0, sizeof(report));
				int sent = write(fd, report, to_send);
				cout<<"data sent:"<<sent<<endl;	
			}
		break;
	}
	return 0;
}



int oActions::mouse_fill_report(char report[8], char buf[BUF_LEN], bool *hold)
{
	char *tok = strtok(buf, " ");
	int mvt = 0;
	int i = 0;
	for (; tok != NULL; tok = strtok(NULL, " ")) {

		if (strcmp(tok, "--quit") == 0)
			return -1;

		if (strcmp(tok, "--hold") == 0) {
			*hold = 1;
			continue;
		}

		for (i = 0; mmod[i].opt != NULL; i++)
			if (strcmp(tok, mmod[i].opt) == 0) {
				report[0] = report[0] | mmod[i].val;
				break;
			}
		if (mmod[i].opt != NULL)
			continue;

		if (!(tok[0] == '-' && tok[1] == '-') && mvt < 2) {
			errno = 0;
			report[1 + mvt++] = (char)strtol(tok, NULL, 0);
			if (errno != 0) {
				report[1 + mvt--] = 0;
			}
			continue;
		}
	}
	return 3;
}



int oActions::joystick_fill_report(char report[8], char buf[BUF_LEN], bool *hold)
{
	char *tok = strtok(buf, " ");
	int mvt = 0;
	int i = 0;

	*hold = 1;

	/* set default hat position: neutral */
	report[3] = 0x04;

	for (; tok != NULL; tok = strtok(NULL, " ")) {

		if (strcmp(tok, "--quit") == 0)
			return -1;

		for (i = 0; jmod[i].opt != NULL; i++)
			if (strcmp(tok, jmod[i].opt) == 0) {
				report[3] = (report[3] & 0xF0) | jmod[i].val;
				break;
			}
		if (jmod[i].opt != NULL)
			continue;

		if (!(tok[0] == '-' && tok[1] == '-') && mvt < 3) {
			errno = 0;
			report[mvt++] = (char)strtol(tok, NULL, 0);
			if (errno != 0) {
				report[mvt--] = 0;
			}
			continue;
		}
	}
	return 4;
}


int oActions::out_thred()
{
	const char *filename = NULL;
	
	char buf[BUF_LEN];
	int cmd_len;
	char report[8];
	int to_send = 8;
	int hold = 0;
	fd_set rfds;
	int retval, i;

	if ((fd = open(hid_name, O_RDWR, 0666)) == -1) {
		return 3;
	}

	while (!stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	close(fd);
	return 0;
}


