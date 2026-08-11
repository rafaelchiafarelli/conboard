// Unit tests for the working-box offset wrapper (LowLevel/HMI/clipped_panel.*).
// A fake PanelDriver stands in for real SPI/GPIO hardware -- this is pure
// delegation/offset-math logic.
#include "doctest.h"          // main() is provided by test_devicedetect.cpp

#include "clipped_panel.hpp"

#include <memory>
#include <vector>

namespace {

// Records every flush() call's coordinates (in the wrapped/physical space)
// so tests can assert on them; reports a fixed physical resolution.
class FakePanel : public PanelDriver {
public:
    FakePanel(int w, int h) : w_(w), h_(h) {}

    bool init() override { return true; }
    int width() const override { return w_; }
    int height() const override { return h_; }
    void flush(int x1, int y1, int x2, int y2, const uint16_t *) override {
        flushes.push_back({x1, y1, x2, y2});
    }
    void setBacklight(bool on) override { backlightOn = on; }

    struct Rect { int x1, y1, x2, y2; };
    std::vector<Rect> flushes;
    bool backlightOn = false;

private:
    int w_, h_;
};

}  // namespace

TEST_SUITE("clipped_panel") {

    TEST_CASE("no offset, full size: reports the inner panel's own resolution") {
        auto fake = std::make_unique<FakePanel>(240, 320);
        FakePanel *fakePtr = fake.get();
        ClippedPanel panel(std::move(fake), 0, 0, 240, 320);
        CHECK(panel.init());
        CHECK(panel.width() == 240);
        CHECK(panel.height() == 320);
        (void)fakePtr;
    }

    TEST_CASE("working box smaller than the panel: width/height reflect the box, not the panel") {
        auto fake = std::make_unique<FakePanel>(240, 320);
        ClippedPanel panel(std::move(fake), 10, 40, 200, 240);
        CHECK(panel.init());
        CHECK(panel.width() == 200);
        CHECK(panel.height() == 240);
    }

    TEST_CASE("flush coordinates are translated by the configured offset") {
        auto fake = std::make_unique<FakePanel>(240, 320);
        FakePanel *fakePtr = fake.get();
        ClippedPanel panel(std::move(fake), 10, 40, 200, 240);
        REQUIRE(panel.init());

        uint16_t pixel = 0;
        panel.flush(0, 0, 9, 9, &pixel);
        REQUIRE(fakePtr->flushes.size() == 1);
        CHECK(fakePtr->flushes[0].x1 == 10);
        CHECK(fakePtr->flushes[0].y1 == 40);
        CHECK(fakePtr->flushes[0].x2 == 19);
        CHECK(fakePtr->flushes[0].y2 == 49);
    }

    TEST_CASE("a working box that exceeds the panel's physical resolution fails init()") {
        auto fake = std::make_unique<FakePanel>(240, 320);
        ClippedPanel tooWide(std::move(fake), 100, 0, 200, 320);   // 100+200 > 240
        CHECK_FALSE(tooWide.init());
    }

    TEST_CASE("a negative offset fails init()") {
        auto fake = std::make_unique<FakePanel>(240, 320);
        ClippedPanel negative(std::move(fake), -1, 0, 240, 320);
        CHECK_FALSE(negative.init());
    }

    TEST_CASE("setBacklight delegates to the wrapped panel") {
        auto fake = std::make_unique<FakePanel>(240, 320);
        FakePanel *fakePtr = fake.get();
        ClippedPanel panel(std::move(fake), 0, 0, 240, 320);
        panel.setBacklight(true);
        CHECK(fakePtr->backlightOn == true);
    }
}
