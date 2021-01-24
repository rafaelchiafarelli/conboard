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
#include <zmq.hpp>
#include <chrono>
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

MIDI::MIDI(string jsonFileName,vector<raw_midi> hw_ports):modes(), header(), json(jsonFileName,&modes,&header),oActions()
{


    /*ZMQ connection -- send user cmds*/
    io_socket.connect("tcp://localhost:5555");
    coms_socket.connect("tcp://localhost:5550");

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
	if ((err = snd_rawmidi_open(&input, &output, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
		std::cout<<"device not found, no thread will be started. err: "<<err<<std::endl;
        in_thread = NULL;
        out_thread = NULL;
	}
    else
    {
        SelectedMode = 0;
        
        for(std::vector<ModeType>::iterator m_it = modes.begin();
            m_it != modes.end();
            m_it++)
        {
            if(m_it->is_active)
            {
                CurrentMode = *m_it;
                break;
            }
            SelectedMode++;
        }
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
void MIDI::handler()
{

    // set up some static data to send
    const std::string data{"Hello"};
    zmq::message_t reply{};
    zmq::recv_result_t recv_res = coms_socket.recv(reply, zmq::recv_flags::none);

    std::cout << "Received " << reply.to_string()<< std::endl;; 
    
    /*msg structure is
    * device cmd params
    * 
    * cmd: reload
    * reload 
    * 
    * cmd file
    * file <complete file_name with path>
    * 
    * cmd outstop
    * outstop
    * 
    */
    std::string raw = reply.to_string();
    
    std::vector<std::string> parsed_msg = explode(raw,' ');
    std::vector<std::string>::iterator msg_it = parsed_msg.begin();
    if(!msg_it->compare(json.DevName))
    {
        msg_it++;
        std::string command = *msg_it;
        if(!command.compare("reload"))
        {
            std::cout<<"Reload!"<<endl;
            Reload();
        }
        else if(!command.compare("outstop"))
        {
            outStop();
        }
        else if(!command.compare("file"))
        {
            msg_it++;
            string param = *msg_it;
            bool isOpen = outFile(param);
            if(isOpen)
                cout<<"file openned"<<endl;
            else
                cout<<"NOT openned"<<endl;
        }
    }
}


std::vector<std::string> MIDI::explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
    }

    return result;
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
    midiActions tmp;
    tmp.midi = midiS;
    std::string snd_data = "{\"";
    snd_data.append(json.DevName);
    snd_data.append("\": \"");
    snd_data.append(std::string(tmp));
    snd_data.append("\"}");

    zmq::send_result_t res = io_socket.send(zmq::buffer(snd_data), zmq::send_flags::dontwait);
    
    if(outToFile)
    {
        outFileStream<<tmp<<endl;
    }

    l_mode.index = SelectedMode;
    l_action.in.mAct.midi = midiS;

    if(CurrentMode.is_active)
    {
        for( std::vector<Actions>::iterator it_act = CurrentMode.body_actions.begin(); it_act != CurrentMode.body_actions.end(); it_act++)
        {
            if((it_act->in.mAct.midi.byte[0] == midiS.byte[0]) &&
               (it_act->in.mAct.midi.byte[1] == midiS.byte[1]) &&
               (it_act->in.mAct.midi.byte[2] == midiS.byte[2]))
            {
                oQueue.push(it_act->out);
                send = true;
                break;
            }
        }
    }
}

void MIDI::out_func()
{
    std::cout<<"out_func start: "<<stop<<" input:"<<input<<std::endl;
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
json.Clear();

}

bool MIDI::outFile(string name)
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


void MIDI::outStop()
{
lock_guard<mutex> locker(locking_mechanism);
outToFile =false;
    if(outFileStream.is_open())
    {
        outFileStream.close();
    }
}


