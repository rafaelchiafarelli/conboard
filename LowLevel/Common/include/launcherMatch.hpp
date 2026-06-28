// Pure device->profile matching for the launcher, split out so it can be unit
// tested. Given the udev variables of an inserted device and a board profile's
// required identifier tags, decide whether the profile applies.
//
// Used by the launcher's create_json(); kept dependency-free (only KeyValue)
// so the runtime decision and the tests share one implementation.
#pragma once

#include "keyParser.hpp"   // KeyValue { std::string key, value; }
#include <vector>

namespace launchmatch {

// True iff EVERY required tag (key+value) is present in deviceVars with an
// equal value. A profile with no tags matches nothing (otherwise a tagless
// profile would claim every device).
bool deviceMatchesTags(const std::vector<KeyValue> &deviceVars,
                       const std::vector<KeyValue> &requiredTags);

} // namespace launchmatch
