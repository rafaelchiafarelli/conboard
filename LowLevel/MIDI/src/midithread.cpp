#include "midithread.hpp"
#include "midiPortMatch.hpp"

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <string>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <algorithm>
#include <unistd.h>

MIDI::MIDI(string _jsonFileName, vector<raw_midi> hw_ports, string usbDevpath)
    : DeviceEngine(_jsonFileName)
{
    memset(port_name, 0, PORT_NAME_SIZE);

    // Identical-MIDI-device separation: prefer the card sitting under this
    // process's bound USB port (usbDevpath, threaded in by the launcher) over a
    // bare name match, so two units of the same controller model each bind their
    // own card instead of both racing for whichever one enumerates first. Falls
    // back to the old plain-name-match automatically when usbDevpath is empty or
    // doesn't line up with anything -- single-unit setups see zero behavior
    // change (LowLevel/Common/midiPortMatch.*).
    std::vector<midiportmatch::PortCandidate> candidates;
    for (vector<raw_midi>::iterator ports_it = hw_ports.begin();
         ports_it != hw_ports.end();
         ports_it++)
    {
        candidates.push_back({ports_it->name, ports_it->port, ports_it->sysfsPath});
    }
    std::string picked = midiportmatch::pickPort(candidates, json.DevName, usbDevpath);
    if (!picked.empty())
        sprintf(port_name, "%s", picked.c_str());

    int err = 0;
    if ((err = snd_rawmidi_open(&input, &output, port_name, SND_RAWMIDI_NONBLOCK)) < 0)
    {
        // No device: leave the engine unstarted (no coms/output/reader threads).
        std::cout << "device not found, no thread will be started. err: " << err << std::endl;
        input = nullptr;
        output = nullptr;
        return;
    }

    // Engine owns the header run, initial mode activation, and the output + coms
    // threads. The MIDI reader is started last, once everything downstream is up.
    startEngine();
    in_thread = new std::thread(&MIDI::in_func, this);
}

MIDI::~MIDI() { Stop(); }

void MIDI::Stop()
{
    stop = true;
    if (in_thread) { in_thread->join(); delete in_thread; in_thread = nullptr; }
    // in_func closes the ALSA handles on exit; nothing to close here.
    stopEngine();
}

// Device-native output: emit raw MIDI (e.g. LED feedback / blink) for MIDI-typed
// outputs only. Called both from the header run (runActions) and from the output
// executor (executeOutput's `case midi`). Non-MIDI outputs are HID and handled by
// the engine, so they are ignored here.
void MIDI::emitNative(const devActions &out)
{
    if (out.tp != devType::midi)
        return;
    if (out.mAct.midi_mode == midi_sysex)
    {
        // Variable-length: send exactly the stored payload, not the fixed
        // 4-byte midiSignal union (unused/zeroed for a sysex-typed output).
        std::vector<uint8_t> payload = out.mAct.sysex;   // local copy, out is const
        send_midi((char *)payload.data(), payload.size());
    }
    else
    {
        midiSignal sig = out.mAct.midi;   // local copy for the ALSA C API (out is const)
        send_midi((char *)sig.byte, sizeof(midiSignal));
    }
    if (out.mAct.delay != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(out.mAct.delay));
}

void MIDI::send_midi(char *send_data, size_t send_data_length)
{
    int err = 0;
    if ((err = snd_rawmidi_nonblock(output, 0)) < 0)
    {
        std::cout << "cannot set blocking mode: " << snd_strerror(err) << std::endl;
    }
    if ((err = snd_rawmidi_write(output, send_data, send_data_length)) < 0)
    {
        std::cout << "cannot send data: " << snd_strerror(err) << std::endl;
    }
}

void MIDI::processInput(midiSignal midiS)
{
    midiActions tmp;
    tmp.midi = midiS;
    // Report the raw input to the dispatcher/UI (+ file logging + overflow msg all
    // handled by the engine's report()).
    report(tmp.ar_str());

    if (CurrentMode.is_active)
    {
        // Matching/output logic lives in the shared, unit-tested midimap module
        // (LowLevel/Common/midiMap.*) so this thread and the tests agree.
        for (std::vector<Actions>::iterator it_act = CurrentMode.body_actions.begin();
             it_act != CurrentMode.body_actions.end(); it_act++)
        {
            if (midimap::matches(it_act->in.mAct, midiS))
            {
                enqueue(midimap::resolveOutputs(*it_act, midiS));
                if (it_act->change_mode && it_act->change_to != -1)
                {
                    changeMode(it_act->change_to);
                }
            }
        }
    }
}

// Same shape as processInput(), for a complete SysEx message (0xF0..0xF7
// inclusive) instead of a fixed 3-byte signal. Exact-match only -- see
// midimap::matchesSysex() -- so no incoming-value pass-through into outputs
// is needed (unlike spot/blink), same as midi_normal today.
void MIDI::processSysex(std::vector<uint8_t> payload)
{
    midiActions tmp;
    tmp.midi_mode = midi_sysex;
    tmp.sysex = payload;
    report(tmp.ar_str());

    if (CurrentMode.is_active)
    {
        for (std::vector<Actions>::iterator it_act = CurrentMode.body_actions.begin();
             it_act != CurrentMode.body_actions.end(); it_act++)
        {
            if (midimap::matchesSysex(it_act->in.mAct, payload))
            {
                enqueue(midimap::resolveOutputs(*it_act, midiSignal{}));
                if (it_act->change_mode && it_act->change_to != -1)
                {
                    changeMode(it_act->change_to);
                }
            }
        }
    }
}

/**
 * Reader thread: reads events from the physical MIDI device and hands each signal
 * to processInput. Unchanged from the original handler (only the surrounding
 * orchestration moved into DeviceEngine).
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

        while (!stop) {

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
                ok = -1;
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

            // SysEx: 0xF0 starts a variable-length message that can span more
            // than one read (a single snd_rawmidi_read() here is capped at
            // sizeof(buf) == 256 bytes; real dumps commonly exceed that).
            // Once started, every subsequent read's bytes belong to the same
            // message until a 0xF7 terminator (or the safety cap) ends it --
            // this branch takes priority over the fixed-3-byte path below.
            if (!inSysex_ && err > 0 && (unsigned char)buf[0] == 0xF0)
            {
                inSysex_ = true;
                sysexBuf_.clear();
            }
            if (inSysex_)
            {
                for (int i = 0; i < err; i++)
                {
                    if (sysexBuf_.size() >= sysexCap_)
                    {
                        // no terminator before the cap -- abandon this message
                        inSysex_ = false;
                        sysexBuf_.clear();
                        break;
                    }
                    sysexBuf_.push_back((uint8_t)buf[i]);
                    if ((unsigned char)buf[i] == 0xF7)
                    {
                        inSysex_ = false;
                        processSysex(std::move(sysexBuf_));
                        sysexBuf_.clear();
                        break;
                    }
                }
                continue;
            }

            if (err > sizeof(midiSignal))
            {
                //investigate this to see if the number of bytes is constant.
                continue;
            }
            //each buf[i] has one of the bytes
            midiSignal midiS;
            midiS.byte[0] = buf[0];
            midiS.byte[1] = buf[1];
            midiS.byte[2] = buf[2];
            midiS.byte[3] = 0;   // was byte[4] — out-of-bounds write (char[4])
            processInput(midiS);
        }
    }
    if (input)
        snd_rawmidi_close(input);
    if (output)
        snd_rawmidi_close(output);
}
