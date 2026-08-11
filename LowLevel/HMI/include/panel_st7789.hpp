// ST7789 4-wire SPI TFT driver (the first concrete PanelDriver -- the exact
// chip for the product isn't picked yet; ST7789 is the most common part in
// this class, see the plan doc). Talks to the panel over /dev/spidevN.N plus
// three GPIO lines (RESET, DC, and an optional BL backlight) via libgpiod.
// Swapping to ST7735/ILI9341 means writing a new PanelDriver implementing the
// same interface, not touching lvgl_glue.cpp or any UI code.
#pragma once

#include "panel_driver.hpp"

#include <cstdint>
#include <string>

struct gpiod_chip;
struct gpiod_line;

class St7789Panel : public PanelDriver {
public:
    struct Config {
        // Defaults below (device, mode, speed) match the known-working
        // reference driver for this exact board+panel
        // (bart-lobotomy/ST7789/ST7789.py on the dev board at 192.168.7.4) --
        // not generic ST7789 assumptions. spiDevice=1.1 in particular is
        // board wiring, not a chip property; verify with `ls /dev/spidev*`
        // if this ever moves to different hardware.
        std::string spiDevice   = "/dev/spidev1.1";
        std::string gpioChip    = "gpiochip0";
        unsigned    resetLine   = 0;
        unsigned    dcLine      = 0;
        int         backlightLine = -1;  // -1: no backlight line wired
        uint32_t    spiSpeedHz  = 4'000'000;
        uint8_t     spiMode     = 3;      // SPI_MODE_3 -- this panel needs it, not mode 0
        int         width       = 240;
        int         height      = 240;
        // Hardware rotation via MADCTL's row/column-exchange bit -- free
        // (no LVGL/CPU cost), unlike a software display rotation. One of
        // 0/90/180/270. UNLIKE the other defaults in this struct, the 90/270
        // MADCTL values are the standard ST7789 convention, not yet
        // hardware-confirmed against this exact panel -- verify orientation
        // on screen and adjust CONHMI_PANEL_ROTATION if it comes out mirrored.
        int         rotation    = 0;
    };

    explicit St7789Panel(Config cfg);
    ~St7789Panel() override;

    bool init() override;
    // Report the POST-rotation resolution (rotation 90/270 swap w/h) -- this
    // is what LVGL and everything built on it (ClippedPanel, AppShell) sees.
    int width() const override;
    int height() const override;
    void flush(int x1, int y1, int x2, int y2, const uint16_t *pixels) override;
    void setBacklight(bool on) override;

private:
    void reset();
    // Full-duplex ioctl(SPI_IOC_MESSAGE) transfer, chunked at 4096 bytes --
    // NOT plain write(2). On this SoC's SPI controller, write(2) to
    // /dev/spidevN.N silently no-ops; the known-working reference driver
    // uses spidev's xfer2() (which is exactly this ioctl) chunked the same
    // way, and that distinction is precisely what separates "commands appear
    // to send with no error" from a panel that actually updates.
    bool spiTransfer(const uint8_t *data, size_t len);
    void writeCommand(uint8_t cmd);
    void writeData(const uint8_t *data, size_t len);
    void writeData(uint8_t byte) { writeData(&byte, 1); }
    void setAddressWindow(int x1, int y1, int x2, int y2);

    Config cfg_;
    int spiFd_ = -1;
    gpiod_chip *chip_ = nullptr;
    gpiod_line *resetLine_ = nullptr;
    gpiod_line *dcLine_ = nullptr;
    gpiod_line *backlightLine_ = nullptr;
};
