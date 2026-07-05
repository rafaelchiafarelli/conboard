#include "deviceEngine.hpp"

#include <chrono>
#include <iostream>

DeviceEngine::DeviceEngine(const std::string &_jsonFileName)
    : oActions(), header(), modes(), json(_jsonFileName, &modes, &header)
{
    jsonFileName = _jsonFileName;
}

DeviceEngine::~DeviceEngine() {
    // Subclasses call stopEngine() themselves (after stopping their reader), but
    // guard here too so a partially-constructed engine still tears down cleanly.
    stopEngine();
}

void DeviceEngine::startEngine() {
    // Run the global header (device feedback at boot).
    runActions(header);

    // Activate the first mode flagged active in the json.
    SelectedMode = 0;
    for (std::vector<ModeType>::iterator m = modes.begin(); m != modes.end(); ++m) {
        if (m->is_active) {
            CurrentMode = *m;
            runActions(CurrentMode.header);
            break;
        }
        SelectedMode++;
    }

    com = new zmq_coms(json.DevName,
                       "tcp://localhost:5551",   // un
                       "tcp://localhost:5552",   // io
                       "tcp://localhost:5550");  // hb
    out_thread = new std::thread(&DeviceEngine::out_func, this);
    thcoms     = new std::thread(&DeviceEngine::coms_handler, this);
}

void DeviceEngine::stopEngine() {
    stop = true;

    if (outToFile)
        outFileStream.close();
    outToFile = false;

    if (out_thread) { out_thread->join(); delete out_thread; out_thread = nullptr; }
    if (thcoms)     { thcoms->join();     delete thcoms;     thcoms     = nullptr; }
    if (com)        { com->die();          delete com;        com        = nullptr; }
}

bool DeviceEngine::report(const std::string &msg) {
    bool ok = com->dispatch(msg);
    if (!ok)
        std::cout << "dispatch overflow" << std::endl;
    if (outToFile)
        outFileStream << msg << std::endl;
    return ok;
}

void DeviceEngine::enqueue(std::vector<devActions> outs) {
    std::lock_guard<std::mutex> locker(locking_mechanism);
    oQueue.push(std::move(outs));
    send = true;
}

void DeviceEngine::runActions(const std::vector<Actions> &actions) {
    for (std::vector<Actions>::const_iterator it = actions.begin(); it != actions.end(); ++it) {
        for (std::vector<devActions>::const_iterator o = it->out.begin(); o != it->out.end(); ++o) {
            devActions out = *o;       // emitNative may want a mutable copy
            emitNative(out);
        }
    }
}

void DeviceEngine::executeOutput(devActions &out) {
    switch (out.tp) {
        case keyboard:
            out.kData.spot = out.spot;
            keyboard_send(out.kData);
            if (out.kData.delay != 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(out.kData.delay));
            break;
        case midi:
            emitNative(out);           // device feedback (MIDI handles its own delay)
            break;
        case mouse:
            oMouse(out.mouse);
            break;
        case joystick:
            oJoystick(out.joy);
            break;
        default:
            break;
    }
}

void DeviceEngine::out_func() {
    while (!stop) {
        if (send) {
            std::vector<devActions> to_send = oQueue.front();
            for (std::vector<devActions>::iterator out = to_send.begin(); out != to_send.end(); ++out)
                executeOutput(*out);
            oQueue.pop();
            if (oQueue.empty())
                send = false;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void DeviceEngine::changeMode(int id) {
    for (std::vector<ModeType>::iterator m = modes.begin(); m != modes.end(); ++m) {
        if (m->index == id) {
            CurrentMode.is_active = false;   // leave the old mode
            CurrentMode = *m;
            runActions(CurrentMode.header);  // bring up the new mode
            CurrentMode.is_active = true;
            return;
        }
    }
}

void DeviceEngine::coms_handler() {
    while (!stop) {
        std::vector<std::string> resp = com->heartbeat();
        std::vector<std::string>::iterator command = resp.begin();   // [0] is the UUID
        if (!resp.empty()) {
            command++;   // [1] is the message body
            if (command->compare("OK")) {   // a non-"OK" reply carries a command
                if (!command->compare("reload")) {
                    Reload();
                } else if (!command->compare("outstop")) {
                    outStop();
                } else if (!command->compare("file")) {
                    command++;
                    outFile(*command);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void DeviceEngine::Reload() {
    std::lock_guard<std::mutex> locker(locking_mechanism);
    while (!oQueue.empty())
        oQueue.pop();
    header.clear();
    modes.clear();
    json.Clear();
    json.Reload(jsonFileName, &modes, &header);
    runActions(header);
}

bool DeviceEngine::outFile(const std::string &name) {
    std::lock_guard<std::mutex> locker(locking_mechanism);
    bool ret = outFileStream.is_open();
    if (!ret) {
        outFileName = name;
        outFileStream.open(name, std::ofstream::out | std::ofstream::app);
        ret = outFileStream.is_open();
        outToFile = ret;
    }
    return ret;
}

void DeviceEngine::outStop() {
    std::lock_guard<std::mutex> locker(locking_mechanism);
    outToFile = false;
    if (outFileStream.is_open())
        outFileStream.close();
}
