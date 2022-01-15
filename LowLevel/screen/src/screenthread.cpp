#include "screenthread.hpp"
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

#include <algorithm>
#include <unistd.h>
#include <zmq.hpp>
#include <chrono>
void TouchScreen::Stop()
{
    stop =true;
    
    if(outToFile)
        {
            outFileStream.close();
        }

    outToFile = false;
    if(in_thread)
         in_thread->join();
    if(out_thread)
        out_thread->join();
    if(thcoms)
        thcoms->join();
        
    com->die();

}
void TouchScreen::parse()
{

}
TouchScreen::~TouchScreen(){
    Stop();
    if(in_thread)
        delete in_thread;
    if(out_thread)
        delete out_thread;
    if(thcoms)
        delete thcoms;
    if(com)
        delete com;
}

TouchScreen::TouchScreen(string _jsonFileName,vector<raw_midi> hw_ports):modes(), header(), json(_jsonFileName,&modes,&header),oActions()
{
    jsonFileName = _jsonFileName;
    outToFile = false;
    memset(port_name,0,PORT_NAME_SIZE);
    for(vector<raw_midi>::iterator ports_it = hw_ports.begin();
        ports_it != hw_ports.end();
        ports_it++)
        {
            if(!json.DevName.compare(ports_it->name))
            {
                sprintf(port_name,"%s",ports_it->port.c_str());
                break;
            }
        }
    stop = false;
    send = false;
    int err = 0;
	if ((err = snd_rawmidi_open(&input, &output, port_name, SND_RAWTouchScreen_NONBLOCK)) < 0) {
		std::cout<<"device not found, no thread will be started. err: "<<err<<std::endl;
        in_thread = NULL;
        out_thread = NULL;
        thcoms = NULL;
        com = NULL;
	}
    else
    {
        execHeader(); //execute the commands in the header
        SelectedMode = 0;
        for(std::vector<ModeType>::iterator m_it = modes.begin();
            m_it != modes.end();
            m_it++)
        {
            if(m_it->is_active)
            {
                CurrentMode = *m_it;
                processMode(CurrentMode);
                break;
            }
            SelectedMode++;
        }
        in_thread = new thread(&TouchScreen::in_func,this);
        out_thread = new thread(&TouchScreen::out_func,this);
        com = new zmq_coms(json.DevName,"tcp://localhost:5551","tcp://localhost:5552", "tcp://localhost:5550");
        thcoms = new thread(&TouchScreen::coms_handler,this);
    }
}


void TouchScreen::execHeader()
{
    for(std::vector<Actions>::iterator it = header.begin();
        it != header.end();
        it++)
    {
        for(std::vector<devActions>::iterator devIt = it->out.begin();
            devIt != it->out.end();
            devIt++)
        {
            if(devIt->tp == midi)
            {
                send_midi(devIt->mAct.midi.byte,sizeof(midiSignal));
                if(devIt->mAct.delay > 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(devIt->mAct.delay));
                }
            }
        }
    }
}

void TouchScreen::coms_handler()
{
    while(!stop)
    {
        std::vector<std::string> resp = com->heartbeat();
        std::vector<std::string>::iterator command = resp.begin(); //begin with the UUID
        if(!resp.empty())
        {
            command++; //get the msg contents
            if(command->compare("OK")) //regular response is OK, not OK, does not mean BAD.
            {
                if(!command->compare("reload"))
                {
                    Reload();
                }
                else if(!command->compare("outstop"))
                {
                    outStop();
                }
                else if(!command->compare("file"))
                {
                    command++;
                    string param = *command;
                    bool isOpen = outFile(param);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}



void TouchScreen::send_mouse(mouseActions mouse)
{
    cout<<mouse<<endl;
}

void TouchScreen::send_midi(char *send_data, size_t send_data_length)
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
void TouchScreen::processMode(ModeType m)
{
    for(std::vector<Actions>::iterator h_it =  m.header.begin();
        h_it != m.header.end();
        h_it++)
    {
        for(std::vector<devActions>::iterator out_it = h_it->out.begin();
            out_it != h_it->out.end();
            out_it++)
        {
            if(out_it->tp == devType::midi)
            {
                send_midi(out_it->mAct.midi.byte,sizeof(midiSignal));
                if(out_it->mAct.delay > 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(out_it->mAct.delay));
                }
            }
        }
    }
}


void TouchScreen::processInput(midiSignal midiS)
{
    lock_guard<mutex> locker(locking_mechanism);
    midiActions tmp;
    tmp.midi = midiS;
    bool res = com->dispatch(tmp.ar_str());
    if(!res)
        std::cout<<"dispatch overflow"<<std::endl;
    if(outToFile)
    {
        outFileStream<<tmp<<endl;
    }
    
    if(CurrentMode.is_active)
    {
        for( std::vector<Actions>::iterator it_act = CurrentMode.body_actions.begin(); it_act != CurrentMode.body_actions.end(); it_act++)
        {
            switch(it_act->in.mAct.midi_mode)
            {
                case midi_normal:
                {
                if((it_act->in.mAct.midi.byte[0] == midiS.byte[0]) &&
                    (it_act->in.mAct.midi.byte[1] == midiS.byte[1]) &&
                    (it_act->in.mAct.midi.byte[2] == midiS.byte[2]))
                    {
                        oQueue.push(it_act->out);
                        send = true;
                        if(it_act->change_mode && it_act->change_to != -1)
                        {
                            changeMode(it_act);
                        }
                    }
                }
                break;
                case midi_trigger_higher:
                {
                if((it_act->in.mAct.midi.byte[0] == midiS.byte[0]) &&
                    (it_act->in.mAct.midi.byte[1] == midiS.byte[1]) &&
                    (it_act->in.mAct.midi.byte[2] < midiS.byte[2]))
                    {
                        oQueue.push(it_act->out);
                        send = true;
                        if(it_act->change_mode && it_act->change_to != -1)
                        {
                            changeMode(it_act);
                        }
                    }
                }
                break;                
                case midi_trigger_lower:
                {
                if((it_act->in.mAct.midi.byte[0] == midiS.byte[0]) &&
                    (it_act->in.mAct.midi.byte[1] == midiS.byte[1]) &&
                    (it_act->in.mAct.midi.byte[2] > midiS.byte[2]))
                    {
                        oQueue.push(it_act->out);
                        send = true;
                        if(it_act->change_mode && it_act->change_to != -1)
                        {
                            changeMode(it_act);
                        }
                    }
                }
                break;
                case midi_spot:
                {
                if((it_act->in.mAct.midi.byte[0] == midiS.byte[0]) &&
                    (it_act->in.mAct.midi.byte[1] == midiS.byte[1]))
                    {
                        for(vector<devActions>::iterator out_it=it_act->out.begin();
                            out_it!=it_act->out.end();
                            out_it++)
                            {
                                out_it->spot = (int) midiS.byte[2];
                            }
                        oQueue.push(it_act->out);
                        send = true;
                        if(it_act->change_mode && it_act->change_to != -1)
                        {
                            changeMode(it_act);
                        }
                    }
                }
                case midi_blink:
                {
                if((it_act->in.mAct.midi.byte[0] == midiS.byte[0]) &&
                    (it_act->in.mAct.midi.byte[1] == midiS.byte[1]) &&
                    (it_act->in.mAct.midi.byte[2] == midiS.byte[2]))
                    {
                        for(vector<devActions>::iterator out_it=it_act->out.begin();
                            out_it!=it_act->out.end();
                            out_it++)
                            {
                                out_it->spot = (int) midiS.byte[2];
                            }
                        oQueue.push(it_act->out);
                        send = true;
                        if(it_act->change_mode && it_act->change_to != -1)
                        {
                            changeMode(it_act);
                        }
                    }
                }                
                break;
            }
        }
    }
}

void TouchScreen::changeMode(std::vector<Actions>::iterator it_act)
{
int id_dest = it_act->change_to;


for(vector<ModeType>::iterator m_it = modes.begin();
    m_it!=modes.end();
    m_it++)
    {


        if(m_it->index == id_dest)
        {
            //Current mode must be turned off, in memory, not in file 

            vector<ModeType>::iterator l_it = modes.begin();
            l_it+=id_dest;
            l_it->is_active = false;

            //changed the mode to the newly selected one
            CurrentMode = *m_it;

            //Activete this new one
            processMode(CurrentMode);

            CurrentMode.is_active=true;
            saveJSON();
        }
    }    
}

void TouchScreen::saveJSON(){

}

void TouchScreen::out_func()
{

    while(!stop)
    {
        if(send)
        {
            std::vector<devActions> to_send = oQueue.front();
            for(std::vector<devActions>::iterator out = to_send.begin();
                out != to_send.end();
                out++)
            {
                switch(out->tp)
                {
                    case keyboard:
                        out->kData.spot = out->spot;
                        keyboard_send(out->kData);
                        if(out->kData.delay != 0)
                            std::this_thread::sleep_for(std::chrono::milliseconds(out->kData.delay));
                        break;
                    case midi:
                        send_midi((char *)out->mAct.midi.byte,sizeof(midiSignal));
                        if(out->mAct.delay !=0)
                             std::this_thread::sleep_for(std::chrono::milliseconds(out->kData.delay));
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
 *      Start by connecting to the device and sending information present at the header section of the json file.
 * 
 * 
 */ 
void TouchScreen::in_func()
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
		
		while(!stop){
            
            unsigned char buf[256];
			int i, length;
			unsigned short revents;
			err = poll(pfds, npfds, MILLISECONDS_TIMEOUT);
			if (stop || (err < 0 && errno == EINTR))
            {
                
                stop = true;
				break;
            }
			if (err < 0) {
				ok=-1;
                stop = true;
                
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
				
				break;
			}
            
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

void TouchScreen::Reload(){
    lock_guard<mutex> locker(locking_mechanism);
    while(!oQueue.empty())
    {
        oQueue.pop();
    }
    header.clear();
    modes.clear();
    json.Clear();
    json.Reload(jsonFileName,&modes,&header);
    execHeader();
}

bool TouchScreen::outFile(string name)
{
lock_guard<mutex> locker(locking_mechanism);
bool ret = outFileStream.is_open();
    if(!ret)
    {
        outFileName = name;
        outFileStream.open(name, std::ofstream::out | std::ofstream::app);
        ret = outFileStream.is_open();
        outToFile = ret;
    }
return ret;
}


void TouchScreen::outStop()
{
lock_guard<mutex> locker(locking_mechanism);
outToFile =false;
    if(outFileStream.is_open())
    {
        outFileStream.close();
    }
}


