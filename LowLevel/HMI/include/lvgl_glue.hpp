// Wires a PanelDriver and the physical input devices into LVGL's lv_display /
// lv_indev model, and drives LVGL's tick/timer loop. This is the ONLY file
// that bridges "hardware" (PanelDriver, RotaryEncoder, PushButton) and LVGL
// -- panel_*.cpp and rotary_encoder.cpp/push_button.cpp know nothing about
// LVGL, and app_shell.cpp knows nothing about hardware.
#pragma once

#include "panel_driver.hpp"
#include "push_button.hpp"
#include "rotary_encoder.hpp"

#include <lvgl.h>

#include <atomic>
#include <cstdint>

namespace lvgl_glue {

// Creates and registers an lv_display sized to panel.width()/height(),
// flushing through panel.flush(). Owns its draw buffers for the lifetime of
// the process (conHMI is single-display, single-process).
lv_display_t *createDisplay(PanelDriver &panel);

// One encoder + its own pushbutton exposed to LVGL as a single ENCODER
// indev in `group`: rotation moves group focus, the button's press/release
// is the indev's PRESSED/RELEASED state -- LVGL's standard group/encoder
// "confirm" semantics (see LV_EVENT_CLICKED in app_shell.cpp's menu items).
lv_indev_t *createEncoderIndev(RotaryEncoder &encoder, PushButton &button, lv_group_t *group);

// A plain button mapped to one LVGL key, delivered as a KEYPAD indev event
// to `group`'s focused widget. Which button maps to which key (e.g.
// LV_KEY_ESC for "back") is a caller (main.cpp / future screens) decision.
lv_indev_t *createButtonIndev(PushButton &button, uint32_t key, lv_group_t *group);

// Runs lv_tick_inc()/lv_timer_handler() on a fixed period until `running`
// becomes false. Blocks the calling thread -- this IS conHMI's main loop.
void runLoop(const std::atomic_bool &running, int periodMs = 5);

} // namespace lvgl_glue
