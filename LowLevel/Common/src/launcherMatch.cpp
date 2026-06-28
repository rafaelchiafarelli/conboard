#include "launcherMatch.hpp"

namespace launchmatch {

bool deviceMatchesTags(const std::vector<KeyValue> &deviceVars,
                       const std::vector<KeyValue> &requiredTags) {
    if (requiredTags.empty())
        return false;   // a profile with no tags matches nothing

    for (const auto &tag : requiredTags) {
        bool found = false;
        for (const auto &v : deviceVars) {
            if (v.key == tag.key && v.value == tag.value) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;   // every required tag must be present and equal
    }
    return true;
}

} // namespace launchmatch
