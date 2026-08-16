#include "midiPortMatch.hpp"
#include "deviceDetect.hpp"   // condetect::nodeUnderUsbPath

namespace midiportmatch {

std::string pickPort(const std::vector<PortCandidate> &candidates,
                      const std::string &devName, const std::string &usbDevpath) {
    if (!usbDevpath.empty()) {
        for (const auto &c : candidates)
            if (c.name == devName && condetect::nodeUnderUsbPath(c.sysfsPath, usbDevpath))
                return c.port;
    }
    // Fallback: today's plain first-name-match, unconditionally available so a
    // devpath that doesn't line up with anything never leaves a single-unit
    // setup unable to bind.
    for (const auto &c : candidates)
        if (c.name == devName)
            return c.port;
    return "";
}

} // namespace midiportmatch
