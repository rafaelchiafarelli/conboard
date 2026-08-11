// libgpiod wrapper around the pure debounce state machine: owns a poll thread
// that reads one GPIO line at a fixed interval and feeds the readings (with a
// real clock) into debounce::update(). See rotary_encoder.hpp for why polling
// (not edge-interrupt events) was chosen here.
#pragma once

#include "debounce.hpp"

#include <atomic>
#include <string>
#include <thread>

struct gpiod_chip;
struct gpiod_line;

class PushButton {
public:
    struct Config {
        std::string gpioChip  = "gpiochip0";
        unsigned    line      = 0;
        bool        activeLow = true;   // typical: button to GND, internal/external pull-up
        long        settleMs  = 20;
        int         pollIntervalUs = 1000;
    };

    explicit PushButton(Config cfg);
    ~PushButton();

    bool start();
    void stop();

    // Current debounced level. Thread-safe: the poll thread writes, the LVGL
    // indev read callback (a different thread) reads.
    bool isPressed() const { return pressed_.load(std::memory_order_relaxed); }

private:
    void pollLoop();

    Config cfg_;
    gpiod_chip *chip_ = nullptr;
    gpiod_line *line_ = nullptr;
    std::thread thread_;
    std::atomic_bool running_{false};
    std::atomic_bool pressed_{false};
    debounce::State debounceState_;
};
