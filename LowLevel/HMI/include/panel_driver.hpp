// Abstract interface for a small SPI TFT panel. An implementation owns the
// hardware handle (spidev fd + gpiod DC/RESET/BL lines, or nothing for the
// no-hardware stub) and translates LVGL's flush requests into panel writes.
// The exact chip (ST7735/ST7789/ILI9341-class) isn't picked yet -- swapping
// one for another means writing a new PanelDriver, not touching any UI code;
// lvgl_glue.cpp only ever talks to this interface.
#pragma once

#include <cstdint>

class PanelDriver {
public:
    virtual ~PanelDriver() = default;

    // Bring the panel out of reset and issue its init command sequence.
    // Returns false if the hardware could not be opened/initialized.
    virtual bool init() = 0;

    // Panel's resolution, valid once init() has succeeded.
    virtual int width() const = 0;
    virtual int height() const = 0;

    // Push one rectangular region of RGB565 pixel data to the panel.
    // (x1,y1)-(x2,y2) are inclusive pixel coordinates, row-major in `pixels`.
    virtual void flush(int x1, int y1, int x2, int y2, const uint16_t *pixels) = 0;

    // Backlight on/off. No-op for drivers without a backlight line.
    virtual void setBacklight(bool on) = 0;
};
