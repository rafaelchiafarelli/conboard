
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
#include <textCharSets.hpp>
using namespace std;

oActions::oActions(char *devName)
{
	fd = open(hid_name, O_RDWR, 0666);
	cout<<"file: "<<hid_name<<" fd:"<<fd<<endl;
}
std::vector<std::string> oActions::words_seperate(std::string s){
    vector<string> ans;
    string w="";
    for(auto i:s){
        if(i==' '){
           ans.push_back(w);
           w="";
        }
        else{
           w+=i;
        }
    }
    ans.push_back(w);
    return ans;
}

void oActions::fillReport(namedKeyCodes key, char *report)
{
	switch(key.number)
	{
		case lAlt:
			report[0] |= 0x04;
		break;
		case rAlt:
			report[0] |= 0x40;
		break;
		case lControl:
			report[0] |= 0x01;
		break;
		case rControl:
			report[0] |= 0x10;
		break;
		case lShift:
			report[0] |= 0x02;
		break;
		case rShift:
			report[0] |= 0x20;
		break;
		default:
			report[3] = key.number; 
		break;
	}
}

/**
 * 
 */
void oActions::sendHotKey(std::vector<std::string> cmds)
{
	char report[8];
	memset(report,0,8);
	size_t to_send = 8;
	for(std::vector<std::string>::iterator str_it = cmds.begin();
	str_it != cmds.end();
	str_it++)
	{
		namedKeyCodes key = onKeySet(str_it->c_str(),(*str_it).size());
		fillReport(key, report);
	}
	int sent = write(fd, report, to_send);
	memset(report, 0x0, sizeof(report));
	sent = write(fd, report, to_send);
}
/**
 * 
 */ 
int oActions::keyboard_send(keyboardActions act)
{
	int key = 0;
	int i = 0;
    char report[8];
    memset(report,0,8);
	size_t to_send = 8;
	
	switch(act.type){

		case hotkey:
			// do stuff
			{
				std::vector<std::string> str_hotkey = words_seperate(act.data);
				sendHotKey(str_hotkey);
			}
		break;
		case text:
			for(std::string::iterator str_it = act.data.begin();
			str_it != act.data.end();
			str_it++
			)
			{
				textCharSet cmd_to_send=textToCmdList[(unsigned int)(*str_it)];
				if(cmd_to_send.cmd)
				{
					namedKeyCodes key=onKeySet(cmd_to_send.cmd,strlen(cmd_to_send.cmd));
					fillReport(key, report);
				}
				if(cmd_to_send.first)
				{
					namedKeyCodes key=onKeySet(cmd_to_send.first,strlen(cmd_to_send.first));
					fillReport(key, report);
				}
				if(cmd_to_send.second)
				{
					namedKeyCodes key=onKeySet(cmd_to_send.second,strlen(cmd_to_send.second));
					fillReport(key, report);
				}
				if(cmd_to_send.third)
				{
					namedKeyCodes key=onKeySet(cmd_to_send.third,strlen(cmd_to_send.third));
					fillReport(key, report);
				}
				if(cmd_to_send.fourth)
				{
					namedKeyCodes key=onKeySet(cmd_to_send.fourth,strlen(cmd_to_send.fourth));
					fillReport(key, report);
				}
				int sent = write(fd, report, to_send);
				memset(report, 0x0, sizeof(report));
				sent = write(fd, report, to_send);
			}

		break;
		case oneKey:
			namedKeyCodes key=onKeySet(act.data.c_str(),act.data.size());
			if(key.number){
				report[3] = key.number;
				to_send = 8;
				int sent_data = write(fd, report, to_send);
				if (act.hold == not_hold) {
					memset(report, 0x0, sizeof(report));
					int sent = write(fd, report, to_send);
				}
				if (act.hold == hold_delay) {
					if(act.delay > 0){
						std::this_thread::sleep_for(std::chrono::microseconds(act.delay));
					}
					memset(report, 0x0, sizeof(report));
					int sent = write(fd, report, to_send);
				}
			}
		break;
	}
	return 0;
}


/*

*/
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


/*

*/
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



