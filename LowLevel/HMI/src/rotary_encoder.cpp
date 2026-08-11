#include "rotary_encoder.hpp"

#include <gpiod.h>

#include <chrono>
#include <iostream>

RotaryEncoder::RotaryEncoder(Config cfg) : cfg_(std::move(cfg)) {}

RotaryEncoder::~RotaryEncoder()
{
    stop();
    if (lineA_) gpiod_line_release(lineA_);
    if (lineB_) gpiod_line_release(lineB_);
    if (chip_) gpiod_chip_close(chip_);
}

bool RotaryEncoder::start()
{
    chip_ = gpiod_chip_open_by_name(cfg_.gpioChip.c_str());
    if (!chip_) {
        std::cerr << "RotaryEncoder: could not open gpio chip " << cfg_.gpioChip << std::endl;
        return false;
    }
    lineA_ = gpiod_chip_get_line(chip_, cfg_.lineA);
    lineB_ = gpiod_chip_get_line(chip_, cfg_.lineB);
    if (!lineA_ || !lineB_) {
        std::cerr << "RotaryEncoder: could not get gpio lines " << cfg_.lineA
                   << "/" << cfg_.lineB << std::endl;
        return false;
    }
    if (gpiod_line_request_input(lineA_, "conHMI-encoder-a") < 0 ||
        gpiod_line_request_input(lineB_, "conHMI-encoder-b") < 0) {
        std::cerr << "RotaryEncoder: could not request gpio lines as input" << std::endl;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&RotaryEncoder::pollLoop, this);
    return true;
}

void RotaryEncoder::stop()
{
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

void RotaryEncoder::pollLoop()
{
    while (running_) {
        int a = gpiod_line_get_value(lineA_);
        int b = gpiod_line_get_value(lineB_);
        if (a >= 0 && b >= 0) {
            quadrature::Step step = quadrature::update(decodeState_, a != 0, b != 0);
            if (step == quadrature::Step::Clockwise)
                pendingSteps_.fetch_add(1, std::memory_order_relaxed);
            else if (step == quadrature::Step::CounterClockwise)
                pendingSteps_.fetch_sub(1, std::memory_order_relaxed);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(cfg_.pollIntervalUs));
    }
}

int RotaryEncoder::consumeSteps()
{
    return pendingSteps_.exchange(0, std::memory_order_relaxed);
}
