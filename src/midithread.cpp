#include "midithread.hpp"
#include <alsa/asoundlib.h>



void MIDI::sig_handler(int dummy){
    stop =true;
}

void MIDI::parse()
{

}


MIDI::MIDI(char *p_name):xml(string("filename"),&modes,&header)
{
    memset(port_name,0,PORT_NAME_SIZE);
    memcpy(port_name,p_name,strlen(p_name));
    int err = 0;
	if ((err = snd_rawmidi_open(&input, &output, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
		//error("cannot open port \"%s\": %s", port_name, snd_strerror(err));
	}
}

void MIDI::execHeader(){
    for(std::vector<Actions>::iterator it = header.begin();
        it != header.end();
        it++)
    {
        for(std::set<devActions,std::greater<devActions>>::iterator devIt = it->out.begin();
            devIt != it->out.end();
            devIt++)
        {
            //outFunction(*devIt);
        }
    }
}

void MIDI::send_midi(char *send_data, size_t send_data_length)
{
    	if (send_data) {
            int err = 0;
		if ((err = snd_rawmidi_nonblock(output, 0)) < 0) {
			//error("cannot set blocking mode: %s", snd_strerror(err));
		}
		if ((err = snd_rawmidi_write(output, send_data, send_data_length)) < 0) {
			//error("cannot send data: %s", snd_strerror(err));
		}
	}
}

int MIDI::processInput()
{

}

void MIDI::thread_func()
{
	int ok = 0;
    int err;
    int lTimeOut = timeout;
	if (input)
		snd_rawmidi_read(input, NULL, 0); /* trigger reading */

	if (input) {
		
		int npfds, time = 0;
		struct pollfd *pfds;

		lTimeOut *= 1000;
		npfds = snd_rawmidi_poll_descriptors_count(input);
		pfds = (pollfd *)alloca(npfds * sizeof(struct pollfd));
		snd_rawmidi_poll_descriptors(input, pfds, npfds);
		
		while(!stop){
			unsigned char buf[256];
			int i, length;
			unsigned short revents;

			err = poll(pfds, npfds, 200);
			if (stop || (err < 0 && errno == EINTR))
				break;
			if (err < 0) {
				ok=-1;
                //error("poll failed: %s", strerror(errno));
				break;
			}
			if (err == 0) {
				time += 200;
				if (lTimeOut && time >= lTimeOut)
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
            //each buf[i] has one of the bytes
            processInput();
		}
	}

	if (input)
		snd_rawmidi_close(input);
	if (output)
		snd_rawmidi_close(output);

	
}

