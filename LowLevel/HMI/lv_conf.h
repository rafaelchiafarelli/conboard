/**
 * lv_conf.h for conHMI (LowLevel/HMI). Tuned for a small SPI TFT
 * (ST7735/ST7789/ILI9341-class) driven from an embedded Linux process on the
 * Orange Pi -- modest memory budget, RGB565 color, no OS integration (conHMI
 * drives LVGL's tick/timer loop itself, see lvgl_glue.cpp). See
 * lvgl/lv_conf_template.h for the full set of options this trims down from.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16   /* RGB565 -- matches every panel in the ST7735/ST7789/ILI9341 class */

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/
#define LV_USE_STDLIB_MALLOC   LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING   LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF  LV_STDLIB_CLIB

/*=========================
   MEMORY SETTINGS
 *=========================*/
/* Unused when LV_USE_STDLIB_MALLOC == LV_STDLIB_CLIB (LVGL calls malloc/free
 * directly), kept here anyway since some LVGL headers still reference it. */
#define LV_MEM_SIZE (64 * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD  30   /* ms; conHMI's own loop drives the real cadence */
#define LV_DPI_DEF 130           /* small-panel-appropriate default DPI for sizing */

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/
#define LV_USE_OS LV_OS_NONE     /* conHMI is single-threaded from LVGL's point of view;
                                     RotaryEncoder/PushButton run their own poll threads
                                     but only ever touch atomics, never LVGL objects */

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1

/* Draw: software rendering only -- no GPU on this class of hardware. */
#define LV_USE_DRAW_SW 1

/*==================
 *   WIDGETS
 *==================*/
#define LV_WIDGETS_HAS_DEFAULT_VALUE 1
#define LV_USE_LABEL     1
#define LV_USE_LIST      1
#define LV_USE_BUTTON    1

/*==================
 * OTHERS
 *==================*/
#define LV_USE_FLEX 1

/*-----------
 * Fonts
 *----------*/
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_DEFAULT &lv_font_montserrat_18

/*==================
 * EXAMPLES AND DEMOS -- off, conHMI doesn't need LVGL's own demo assets
 *==================*/
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0

#endif /*LV_CONF_H*/
