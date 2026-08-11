#include "push_button.hpp"

#include <gpiod.h>

#include <chrono>
#include <iostream>

PushButton::PushButton(Config cfg) : cfg_(std::move(cfg)) {}

PushButton::~PushButton()
{
    stop();
    if (line_) gpiod_line_release(line_);
    if (chip_) gpiod_chip_close(chip_);
}

bool PushButton::start()
{
    chip_ = gpiod_chip_open_by_name(cfg_.gpioChip.c_str());
    if (!chip_) {
        std::cerr << "PushButton: could not open gpio chip " << cfg_.gpioChip << std::endl;
        return false;
    }
    line_ = gpiod_chip_get_line(chip_, cfg_.line);
    if (!line_) {
        std::cerr << "PushButton: could not get gpio line " << cfg_.line << std::endl;
        return false;
    }
    if (gpiod_line_request_input(line_, "conHMI-button") < 0) {
        std::cerr << "PushButton: could not request gpio line " << cfg_.line << " as input" << std::endl;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&PushButton::pollLoop, this);
    return true;
}

void PushButton::stop()
{
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

void PushButton::pollLoop()
{
    const auto start = std::chrono::steady_clock::now();
    debounce::Config cfg{cfg_.settleMs};

    while (running_) {
        int raw = gpiod_line_get_value(line_);
        if (raw >= 0) {
            bool level = cfg_.activeLow ? (raw == 0) : (raw != 0);
            long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start).count();
            debounce::Edge edge = debounce::update(debounceState_, level, now, cfg);
            if (edge == debounce::Edge::Pressed)
                pressed_.store(true, std::memory_order_relaxed);
            else if (edge == debounce::Edge::Released)
                pressed_.store(false, std::memory_order_relaxed);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(cfg_.pollIntervalUs));
    }
}
