// A small custom LVGL theme so the HMI reads as the same product as the
// console, not a stock LVGL demo. The palette below is a direct copy of the
// console's dark/amber tokens (frontend/console/src/index.css :root) --
// keep the two in sync if that file's palette ever changes.
//
// Deliberately narrow: it only styles the widget kinds app_shell.cpp
// actually builds (screens, plain containers, lv_list + its buttons). Add
// cases to theme_apply() in the .cpp as future screens introduce new widget
// kinds (arc, roller, ...) rather than pre-covering LVGL's whole widget set.
#pragma once

#include <lvgl.h>

namespace hmi_theme {

// Call once, right after lvgl_glue::createDisplay() and before any screen/
// widget is created (theme application happens at object-creation time).
void init(lv_display_t *disp);

} // namespace hmi_theme
