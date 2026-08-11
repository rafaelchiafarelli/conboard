// Wraps a PanelDriver to expose only a sub-rectangle of its full physical
// addressable area as the "working box". LVGL and everything built on top
// (app_shell.cpp, main.cpp's screens) only ever see the working box's
// width()/height(); every flush is translated back into the physical
// panel's coordinate space before being handed to the wrapped driver.
//
// Exists because the enclosure this panel mounts into crops the usable area
// to something smaller than (and offset within) the panel's full native
// resolution, and that crop isn't fixed/known at compile time -- it's
// configured at startup (see main.cpp's CONHMI_WORK_* env vars) so it can be
// tuned without a rebuild while the enclosure is still being worked out. No
// other file needs to know this wrapping exists: it's just another
// PanelDriver, which is the whole point of the interface.
#pragma once

#include "panel_driver.hpp"

#include <memory>

class ClippedPanel : public PanelDriver {
public:
    // inner: the real panel driver (ownership taken). xOffset/yOffset/width/
    // height: the working box, in inner's own coordinate space.
    ClippedPanel(std::unique_ptr<PanelDriver> inner, int xOffset, int yOffset,
                 int width, int height);

    // Delegates to inner.init(), after validating the working box actually
    // fits inner's physical resolution -- fails loudly (false + logged
    // reason) rather than silently writing out of bounds on a misconfigured
    // box, since that box is exactly the thing expected to need iterating on.
    bool init() override;
    int width() const override { return width_; }
    int height() const override { return height_; }
    void flush(int x1, int y1, int x2, int y2, const uint16_t *pixels) override;
    void setBacklight(bool on) override;

private:
    std::unique_ptr<PanelDriver> inner_;
    int xOffset_;
    int yOffset_;
    int width_;
    int height_;
};
