// Unit tests for the identical-MIDI-device port picker. Pure logic, synthetic
// candidates -- no ALSA, no real /sys.
#include "doctest.h"

#include "midiPortMatch.hpp"

using namespace midiportmatch;

TEST_SUITE("midiport") {
    const std::string portA = "/devices/platform/soc/1c1b000.usb/usb1/1-1/1-1.2";
    const std::string portB = "/devices/platform/soc/1c1b000.usb/usb1/1-1/1-1.3";
    const std::string sysfsA =
        "/sys/devices/platform/soc/1c1b000.usb/usb1/1-1/1-1.2/1-1.2:1.3/sound/card1";
    const std::string sysfsB =
        "/sys/devices/platform/soc/1c1b000.usb/usb1/1-1/1-1.3/1-1.3:1.3/sound/card2";

    TEST_CASE("identity picks the candidate under the given USB port") {
        std::vector<PortCandidate> cands = {
            {"DJ-Tech 4-Mix", "hw:1,0,0", sysfsA},
            {"DJ-Tech 4-Mix", "hw:2,0,0", sysfsB},
        };
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", portB) == "hw:2,0,0");
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", portA) == "hw:1,0,0");
    }

    TEST_CASE("no devpath falls back to first name match (today's behavior)") {
        std::vector<PortCandidate> cands = {
            {"DJ-Tech 4-Mix", "hw:1,0,0", sysfsA},
            {"DJ-Tech 4-Mix", "hw:2,0,0", sysfsB},
        };
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", "") == "hw:1,0,0");
    }

    TEST_CASE("devpath given but nothing sits under it still falls back") {
        std::vector<PortCandidate> cands = {
            {"DJ-Tech 4-Mix", "hw:1,0,0", sysfsA},
        };
        const std::string unrelated = "/devices/platform/soc/1c1b000.usb/usb1/1-1/1-1.9";
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", unrelated) == "hw:1,0,0");
    }

    TEST_CASE("no name match at all -> empty") {
        std::vector<PortCandidate> cands = {
            {"Some Other Controller", "hw:1,0,0", sysfsA},
        };
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", "").empty());
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", portA).empty());
    }

    TEST_CASE("empty candidate list -> empty") {
        std::vector<PortCandidate> cands;
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", "").empty());
    }

    TEST_CASE("a single unit's sub-devices: identity picks the right sub") {
        // Same card/physical port, different ALSA subdevices -- identity alone
        // does not disambiguate those (by design; that's not this bug), so the
        // first name+identity match under the port wins, same as plain name
        // match would for a single unit.
        std::vector<PortCandidate> cands = {
            {"DJ-Tech 4-Mix", "hw:1,0,0", sysfsA},
            {"DJ-Tech 4-Mix", "hw:1,0,1", sysfsA},
        };
        CHECK(pickPort(cands, "DJ-Tech 4-Mix", portA) == "hw:1,0,0");
    }
}
