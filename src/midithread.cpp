#include "midithread.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <alsa/asoundlib.h>




void MIDI::Stop()
{
    stop =true;
}
void MIDI::parse()
{

}
MIDI::~MIDI(){
    in_thread->join();
    out_thread->join();
    delete in_thread;
    delete out_thread;
}

MIDI::MIDI(char *p_name, string xmlFileName):xml(xmlFileName,&modes,&header)
{
    memset(port_name,0,PORT_NAME_SIZE);
    memcpy(port_name,p_name,strlen(p_name));
    int err = 0;
	if ((err = snd_rawmidi_open(&input, &output, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
		//error("cannot open port \"%s\": %s", port_name, snd_strerror(err));
	}
    SelectedMode = 0;
    in_thread = new thread(&MIDI::in_func,this);
    out_thread = new thread(&MIDI::out_func,this);
}

void MIDI::execHeader()
{
    for(std::vector<Actions>::iterator it = header.begin();
        it != header.end();
        it++)
    {
        for(std::vector<devActions>::iterator devIt = it->out.begin();
            devIt != it->out.end();
            devIt++)
        {
            send_midi(devIt->midi.byte,sizeof(midiSignal));
            if(devIt->delay > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(devIt->delay));
        }
    }
}

void MIDI::send_midi(char *send_data, size_t send_data_length)
{
    int err = 0;
    if ((err = snd_rawmidi_nonblock(output, 0)) < 0) 
    {
        //error("cannot set blocking mode: %s", snd_strerror(err));
    }
    if ((err = snd_rawmidi_write(output, send_data, send_data_length)) < 0) 
    {
        //error("cannot send data: %s", snd_strerror(err));
    }
}

int MIDI::processInput(midiSignal midiS)
{
    l_mode.index = SelectedMode;
    l_action.in.midi = &midiS;
    std::set<ModeType, std::greater<ModeType>>::iterator it_mode =modes.find(l_mode);
    std::set<Actions, std::greater<Actions>>::iterator it_act = it_mode->body_actions.find(l_action);
    if(it_act)
    {
        for(std::vector<devActions>::iterator it_out = it_act->out.begin();
            it_out != it_act->out.end();
            it_out++)
        {
            send_midi(it_out->midi.byte,sizeof(midiSignal));
            if(it_out->delay > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(it_out->delay));
        }
    }
}

void MIDI::out_func()
{
    while(!stop)
    {
        //read from queue and launch it to the device.
        //sleeps otherwise
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
/**
 * 
 * This is the thread that will read the input's from user and queue the outputs
 *      Start by connecting to the device and sending information present at the header section of the XML file.
 * 
 * 
 */ 
void MIDI::in_func()
{
	int ok = 0;
    int err;
    int lTimeOut = timeout;
	if (input)
		snd_rawmidi_read(input, NULL, 0); /* trigger reading */

	if (input) {
		
		int npfds, time = 0;
		struct pollfd *pfds;

		npfds = snd_rawmidi_poll_descriptors_count(input);
		pfds = (pollfd *)alloca(npfds * sizeof(struct pollfd));
		snd_rawmidi_poll_descriptors(input, pfds, npfds);
		execHeader(); //execute the commands in the header
		while(!stop){
			unsigned char buf[256];
			int i, length;
			unsigned short revents;

			err = poll(pfds, npfds, MILLISECONDS_TIMEOUT);
			if (stop || (err < 0 && errno == EINTR))
				break;
			if (err < 0) {
				ok=-1;
                //error("poll failed: %s", strerror(errno));
				break;
			}
			if (err == 0) {
				time += MILLISECONDS_TIMEOUT;
				if (time >= lTimeOut)
                {
                    ok = -1;
					break;
                }
				continue;
			}
			if ((err = snd_rawmidi_poll_descriptors_revents(input, pfds, npfds, &revents)) < 0) {
                ok = -1;
				//error("cannot get poll events: %s", snd_strerror(errno));
				break;
			}
			if (revents & (POLLERR | POLLHUP))
            {
                ok = -1;
				break;
            }
			if (!(revents & POLLIN))
				continue;
			err = snd_rawmidi_read(input, buf, sizeof(buf));
			if (err == -EAGAIN)
				continue;
			if (err < 0) {
                ok = -1;
				//error("cannot read from port \"%s\": %s", port_name, snd_strerror(err));
				break;
			}

			time = 0;
            if(err > sizeof(midiSignal))
            {
                //investigate this to see if the number of bytes is constant.
                continue;                
            }
            //each buf[i] has one of the bytes
            midiSignal midiS = static_cast<midiSignal> &buf[0];
            processInput(midiS);
		}
	}

	if (input)
		snd_rawmidi_close(input);
	if (output)
		snd_rawmidi_close(output);

	
}

