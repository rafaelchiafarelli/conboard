/**
 * MIDI input handler -- now a thin subclass of DeviceEngine.
 *
 * A device is a set of boot-up actions plus a set of event-driven actions. As the
 * physical MIDI device sends input signals, they are matched against the current
 * mode's rules and turned into outputs (keyboard/mouse HID, or MIDI feedback back
 * to the sender -- e.g. an LED blink).
 *
 * Everything that does NOT depend on the MIDI wire format lives in DeviceEngine:
 *   - the output queue + executor thread (keyboard/mouse/joystick HID),
 *   - the backend coms/heartbeat loop (reload / file / outstop commands),
 *   - the mode model + mode switching, Reload, and file logging.
 * MIDI adds only:
 *   - its INPUT layer: the ALSA rawmidi reader (in_func) + the pure midimap matcher,
 *   - its device-native OUTPUT: emitNative() -> raw MIDI (LED feedback / blink).
 */

#ifndef MIDITHREAD_HPP
#define MIDITHREAD_HPP

#include "deviceEngine.hpp"
#include "actions.h"
#include "midiMap.hpp"
#include "aconfig.h"

#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include <thread>

using namespace std;
#define PORT_NAME_SIZE 10
#define MILLISECONDS_TIMEOUT 10

class raw_midi{
    public:
        string port;
        string devName;
        string sub_name;
        string name;
        int sub;
        int card;
        int device;
    friend std::ostream& operator<<(std::ostream &os, const raw_midi &dt){
        os<<"port:"<<dt.port<<" devName:"<<dt.devName<<" sub_name:"<<dt.sub_name<<" name:"<<dt.name;
        return os;
    };
};

class MIDI : public DeviceEngine {
    public:
        MIDI(string jsonFileName, vector<raw_midi> hw_ports);
        ~MIDI() override;

        // Stop the reader, then tear down the engine. Idempotent.
        void Stop();

    protected:
        // Device-native feedback: only MIDI-typed outputs are emitted here (raw
        // MIDI to the sender); keyboard/mouse HID are handled generically by the
        // engine's output executor.
        void emitNative(const devActions &out) override;

    private:
        void in_func();                         // ALSA rawmidi reader thread
        void processInput(midiSignal midiS);    // report + match + enqueue
        void send_midi(char *send_data, size_t send_data_length);

        std::thread   *in_thread = nullptr;
        snd_rawmidi_t *input  = nullptr;
        snd_rawmidi_t *output = nullptr;
        char           port_name[PORT_NAME_SIZE];
        // Reader idle budget. Kept for parity with the original reader loop; its
        // value is inconsequential (both branches on a poll timeout `continue`).
        int            timeout = 0;
};

#endif
