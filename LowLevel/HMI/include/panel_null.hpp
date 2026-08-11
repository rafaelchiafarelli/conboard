// No-hardware stand-in for PanelDriver: reports a caller-chosen resolution and
// (optionally) dumps every flushed frame to a PPM file. Lets the LVGL app,
// AppShell components, and the REST-driven demo screen be built and run with
// zero SPI/GPIO hardware -- including in a plain dev environment.
#pragma once

#include "panel_driver.hpp"

#include <cstdint>
#include <string>
#include <vector>

class NullPanel : public PanelDriver {
public:
    // dumpPath: if non-empty, every flush() rewrites this file with a binary
    // (P6) PPM snapshot of the full frame, so the render can be eyeballed
    // (any image viewer opens PPM) without a physical panel.
    NullPanel(int width, int height, std::string dumpPath = "");

    bool init() override;
    int width() const override { return w_; }
    int height() const override { return h_; }
    void flush(int x1, int y1, int x2, int y2, const uint16_t *pixels) override;
    void setBacklight(bool /*on*/) override {}

private:
    void dumpFrame() const;

    int w_;
    int h_;
    std::string dumpPath_;
    std::vector<uint16_t> frame_;  // full-frame shadow buffer, RGB565
};
