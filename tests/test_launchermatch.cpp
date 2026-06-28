// Unit tests for the launcher's device->profile tag matching. Pure: synthetic
// KeyValue vectors, no udev, no filesystem.
#include "doctest.h"          // main() provided by test_devicedetect.cpp

#include "launcherMatch.hpp"

using namespace launchmatch;

static KeyValue kv(std::string k, std::string v) {
    KeyValue x; x.key = std::move(k); x.value = std::move(v); return x;
}

TEST_SUITE("launch") {
    TEST_CASE("all required tags present (among extra device vars) -> match") {
        std::vector<KeyValue> dev = { kv("ACTION","add"), kv("ID_MODEL","DJ-Tech_4-Mix"),
                                      kv("ID_VENDOR","x") };
        CHECK(deviceMatchesTags(dev, { kv("ID_MODEL","DJ-Tech_4-Mix") }));
    }

    TEST_CASE("a required tag absent -> no match") {
        std::vector<KeyValue> dev = { kv("ID_VENDOR","x") };
        CHECK_FALSE(deviceMatchesTags(dev, { kv("ID_MODEL","DJ-Tech_4-Mix") }));
    }

    TEST_CASE("right key, wrong value -> no match") {
        std::vector<KeyValue> dev = { kv("ID_MODEL","SomethingElse") };
        CHECK_FALSE(deviceMatchesTags(dev, { kv("ID_MODEL","DJ-Tech_4-Mix") }));
    }

    TEST_CASE("multiple tags: all must match") {
        std::vector<KeyValue> dev = { kv("ID_MODEL","m"), kv("ID_VENDOR","v") };
        CHECK(deviceMatchesTags(dev, { kv("ID_MODEL","m"), kv("ID_VENDOR","v") }));
        CHECK_FALSE(deviceMatchesTags(dev, { kv("ID_MODEL","m"), kv("ID_VENDOR","WRONG") }));
    }

    TEST_CASE("empty required tags -> no match (tagless profile claims nothing)") {
        CHECK_FALSE(deviceMatchesTags({ kv("ID_MODEL","m") }, {}));
    }

    TEST_CASE("empty device vars + non-empty tags -> no match") {
        CHECK_FALSE(deviceMatchesTags({}, { kv("ID_MODEL","m") }));
    }
}
