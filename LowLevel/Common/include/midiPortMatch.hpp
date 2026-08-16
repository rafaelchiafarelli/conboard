// Pure ALSA-rawmidi port selection, split out so it can be unit tested without
// linking ALSA -- mirrors launcherMatch.hpp's shape (a small, dependency-light
// module the runtime decision and the tests share). Solves the identical-MIDI-
// device problem: ALSA rawmidi binds by name, so two units of the same
// controller model report the same name and would otherwise collide on
// whichever one enumerates first.
#pragma once

#include <string>
#include <vector>

namespace midiportmatch {

// One enumerated ALSA rawmidi port, decoupled from ALSA types (mirrors
// raw_midi's name/port/sysfsPath fields) so selection stays pure/testable.
struct PortCandidate {
    std::string name;       // ALSA-reported name (snd_rawmidi_info_get_name)
    std::string port;       // "hw:card,device,sub"
    std::string sysfsPath;  // resolved /sys/class/sound/cardN path
};

// Pick the port to bind to:
//   1. If usbDevpath is non-empty: prefer the candidate whose name matches
//      devName AND whose sysfsPath sits under usbDevpath (condetect::
//      nodeUnderUsbPath) -- the identity check that separates two identical-
//      model units on different physical ports.
//   2. Otherwise, or if nothing satisfies step 1: fall back to the first
//      candidate whose name matches devName -- today's exact algorithm, so
//      single-unit setups see zero behavior change even once a devpath is
//      threaded in.
//   3. No name match at all -> "".
std::string pickPort(const std::vector<PortCandidate> &candidates,
                      const std::string &devName, const std::string &usbDevpath);

} // namespace midiportmatch
