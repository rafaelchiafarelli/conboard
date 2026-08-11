// libgpiod wrapper around the pure quadrature decoder: owns a poll thread
// that reads two GPIO lines (A/B) at a tight, fixed interval and feeds every
// reading into quadrature::update(). A tight poll (rather than edge-interrupt
// events) is a deliberate simplicity trade-off -- plenty fast for a
// human-turned UI encoder, and it keeps this file to the same handful of
// well-known libgpiod v1 calls PushButton and the ST7789 driver use.
#pragma once

#include "quadrature.hpp"

#include <atomic>
#include <string>
#include <thread>

struct gpiod_chip;
struct gpiod_line;

class RotaryEncoder {
public:
    struct Config {
        std::string gpioChip = "gpiochip0";
        unsigned    lineA    = 0;
        unsigned    lineB    = 0;
        int         pollIntervalUs = 500;
    };

    explicit RotaryEncoder(Config cfg);
    ~RotaryEncoder();

    // Opens the gpio lines and starts the poll thread. False on any gpio failure.
    bool start();
    void stop();

    // Net steps (positive = clockwise) accumulated since the last call; resets
    // to 0 on read. Thread-safe: the poll thread writes, the LVGL indev read
    // callback (a different thread) consumes.
    int consumeSteps();

private:
    void pollLoop();

    Config cfg_;
    gpiod_chip *chip_  = nullptr;
    gpiod_line *lineA_ = nullptr;
    gpiod_line *lineB_ = nullptr;
    std::thread thread_;
    std::atomic_bool running_{false};
    std::atomic_int  pendingSteps_{0};
    quadrature::State decodeState_;
};
