#include "midithread.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <alsa/asoundlib.h>
#include <string>

#include <iomanip>

#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <unistd.h>


void MIDI::Stop()
{
    stop =true;
    if(outToFile)
        {
            outFileStream.close();
            
        }

    outToFile = false;
    if(in_thread)
        if(in_thread->joinable())
            in_thread->join();
    if(out_thread)
        if(out_thread->joinable())
            out_thread->join();

}
void MIDI::parse()
{

}
MIDI::~MIDI(){
    Stop();
    if(in_thread)
        delete in_thread;
    if(out_thread)
        delete out_thread;
}

MIDI::MIDI(char *p_name, string xmlFileName, char *devName ):modes(), header(), xml(xmlFileName.c_str(),&modes,&header),oActions()
{
    outToFile = false;
    memset(port_name,0,PORT_NAME_SIZE);
    memcpy(port_name,p_name,strlen(p_name));
    stop = false;
    send = false;
    int err = 0;
    std::cout<<"MIDI"<<std::endl;
	if ((err = snd_rawmidi_open(&input, &output, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
		std::cout<<"device not found, no thread will be started. err: "<<err<<std::endl;
        in_thread = NULL;
        out_thread = NULL;
	}
    else
    {
        SelectedMode = 0;
        in_thread = new thread(&MIDI::in_func,this);
        out_thread = new thread(&MIDI::out_func,this);
    }
}

void MIDI::execHeader()
{
    for(std::vector<Actions>::iterator it = header.begin();
        it != header.end();
        it++)
    {
        //std::cout<<"header"<<std::endl;
        for(std::vector<devActions>::iterator devIt = it->out.begin();
            devIt != it->out.end();
            devIt++)
        {
            //std::cout<<"devIt:  "<<devIt->tp<<std::endl;
            if(devIt->tp == midi)
            {
                send_midi(devIt->mAct.midi.byte,sizeof(midiSignal));
                if(devIt->mAct.delay > 0)
                {
                    //std::cout<<"thi is a sleep:"<<devIt->delay<<std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(devIt->mAct.delay));
                }
            }
        }
    }
}



void MIDI::send_mouse(mouseActions mouse)
{
    cout<<mouse<<endl;
}

void MIDI::send_midi(char *send_data, size_t send_data_length)
{
    int err = 0;
    if ((err = snd_rawmidi_nonblock(output, 0)) < 0) 
    {
        std::cout<<"cannot set blocking mode: "<<snd_strerror(err)<<std::endl;
    }
    if ((err = snd_rawmidi_write(output, send_data, send_data_length)) < 0) 
    {
        std::cout<<"cannot send data: "<<snd_strerror(err)<<std::endl;
    }
}

void MIDI::processInput(midiSignal midiS)
{
    lock_guard<mutex> locker(locking_mechanism);

    if(outToFile)
    {
        outFileStream<<midiS.byte<<endl;
    }

    l_mode.index = SelectedMode;
    l_action.in.mAct.midi = midiS;

    std::vector<ModeType>::iterator it_mode =modes.begin();
    
    std::vector<Actions>::iterator it_act = it_mode->body_actions.begin();
    
    if(it_mode != modes.end())
    {
        cout<<"mode is valid"<<endl;
        if(it_act != it_mode->body_actions.end())
        {
            cout<<"in is:"<<it_act->in<<endl;
            for(std::vector<devActions>::iterator it_tmp = it_act->out.begin();
                it_tmp != it_act->out.end();
                it_tmp++)
            {
                cout<<"out: "<<*it_tmp<<endl;
            }
            oQueue.push(it_act->out);
            send = true;
        }
    }
}

void MIDI::out_func()
{
    std::cout<<"out_func start: "<<stop<<" input:"<<input<<std::endl;
    while(!stop)
    {
        //read from queue and launch it to the device.
        //sleeps otherwise
        
        if(send)
        {
            cout<<"out_function"<<endl;
            std::vector<devActions> to_send = oQueue.front();
            for(std::vector<devActions>::iterator out = to_send.begin();
                out != to_send.end();
                out++)
                {
                    std::cout<<"out:"<<*out<<std::endl;
                    switch(out->tp)
                        {
                            case keyboard:
                                keyboard_send(out->kData);
                                std::this_thread::sleep_for(std::chrono::milliseconds(out->kData.delay));
                                break;
                            case midi:
                                send_midi((char *)out->mAct.midi.byte,sizeof(midiSignal));
                                std::this_thread::sleep_for(std::chrono::milliseconds(out->mAct.delay));
                                break;
                            case mouse:
                                send_mouse(out->mouse);
                                break;
                            case joystick:
                                send_joystick();
                                break;
                            default:
                                break;
                        }
                }
                oQueue.pop();
                if(oQueue.empty())
                {
                    send = false;
                }
        }    
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
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
    std::cout<<"in_func start: "<<stop<<" input:"<<input<<std::endl;
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
            {
                std::cout<<"poll failed: "<<stop<<" "<<err<<std::endl;
                stop = true;
				break;
            }
			if (err < 0) {
				ok=-1;
                stop = true;
                std::cout<<"poll failed: "<<strerror(errno)<<std::endl;
				break;
			}
			if (err == 0) {
				time += MILLISECONDS_TIMEOUT;
				if (time >= lTimeOut)
                {
					continue;
                }
			}
			if ((err = snd_rawmidi_poll_descriptors_revents(input, pfds, npfds, &revents)) < 0) {
                ok = -1;
				std::cout<<"cannot get poll events: "<<snd_strerror(errno)<<std::endl;
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
				std::cout<<"cannot read from port "<<port_name<<" , "<<snd_strerror(err)<<std::endl;
				break;
			}
            std::cout<<std::hex<<(unsigned int)buf[0]<<" "<<std::hex<<(unsigned int)buf[1]<<" "<<std::hex<<(unsigned int)buf[2]<<std::endl;
			time = 0;
            if(err > sizeof(midiSignal))
            {
                //investigate this to see if the number of bytes is constant.
                continue;                
            }
            //each buf[i] has one of the bytes
            midiSignal midiS;
            midiS.byte[0] = buf[0];
            midiS.byte[1] = buf[1];
            midiS.byte[2] = buf[2];
            midiS.byte[4] = 0;
            
            processInput(midiS);
		}
	}
	if (input)
		snd_rawmidi_close(input);
	if (output)
		snd_rawmidi_close(output);
    
}

void MIDI::Reload(){
lock_guard<mutex> locker(locking_mechanism);
while(!oQueue.empty())
    oQueue.pop();

header.clear();
modes.clear();
xml.Clear();

}

bool MIDI::outFile(string name)
{
lock_guard<mutex> locker(locking_mechanism);
bool ret = false;
    if(!outFileStream.is_open())
    {
        outFileName = name;
        outFileStream.open(name, std::ofstream::out | std::ofstream::app);
        ret = outFileStream.is_open();
        outToFile = ret;
    }
return ret;
}


void MIDI::outStop()
{
lock_guard<mutex> locker(locking_mechanism);
outToFile =false;
    if(outFileStream)
    {
        outFileStream.close();
    }
}


