// Small reusable "shell" of generic LVGL components any future screen (WiFi
// list, radio list, settings, activation) can be built from. No domain
// content lives here -- that's phase 4; this is purely navigation/menu/value
// scaffolding, built against LVGL's own resolution/DPI model so it adapts to
// whatever resolution PanelDriver::width()/height() report, with no
// per-resolution branching.
//
// Which physical control drives what (e.g. "the left encoder scrolls this
// menu") is deliberately NOT decided here -- lvgl_glue.cpp exposes each
// RotaryEncoder/PushButton as a clean LVGL indev, and it's a phase-4 app
// decision which indev goes into which screen's group. ValueRow mirrors
// that: it exposes adjust()/onChange, but nothing in this file reads an
// encoder directly.
#pragma once

#include <lvgl.h>

#include <functional>
#include <string>
#include <vector>

namespace appshell {

// Owns a simple screen stack: push shows a new blank full-screen container
// and makes it active; pop deletes the active screen and reactivates the
// previous one (a no-op if only one screen remains).
class Shell {
public:
    explicit Shell(lv_group_t *group);

    lv_obj_t *pushScreen();
    void popScreen();

    lv_group_t *group() const { return group_; }

private:
    lv_group_t *group_;
    std::vector<lv_obj_t *> stack_;
};

// A scrollable, encoder-navigable list of selectable rows (built on lv_list).
// Each item added via addMenuItem() joins the shell's focus group, so an
// ENCODER indev in that group can move between rows and "press" activates
// the focused one (LVGL's standard group/encoder confirm semantics).
lv_obj_t *createMenuList(Shell &shell, lv_obj_t *parent);
lv_obj_t *addMenuItem(Shell &shell, lv_obj_t *list, const std::string &text,
                       std::function<void()> onActivate);

// A label + value display row (e.g. "Volume   7"). adjust() changes the
// shown value and fires onChange -- something else (a future screen) decides
// when/how much to call adjust() with, e.g. from an encoder's consumeSteps().
class ValueRow {
public:
    ValueRow(lv_obj_t *parent, const std::string &label, int initial,
              int minVal, int maxVal, std::function<void(int)> onChange);

    void adjust(int delta);
    int value() const { return value_; }
    lv_obj_t *container() const { return container_; }

private:
    void refreshLabel();

    lv_obj_t *container_;
    lv_obj_t *valueLabel_;
    int value_;
    int minVal_;
    int maxVal_;
    std::function<void(int)> onChange_;
};

// A plain, unfocusable text label -- e.g. the console-URL demo screen.
lv_obj_t *createInfoLabel(lv_obj_t *parent, const std::string &text);

} // namespace appshell
