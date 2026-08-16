#include "lvgl_glue.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace lvgl_glue {

namespace {

void flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    auto *panel = static_cast<PanelDriver *>(lv_display_get_user_data(disp));
    panel->flush(area->x1, area->y1, area->x2, area->y2,
                 reinterpret_cast<const uint16_t *>(px_map));
    lv_display_flush_ready(disp);
}

struct EncoderCtx {
    RotaryEncoder *encoder;
    PushButton *button;
};
void encoderReadCb(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto *ctx = static_cast<EncoderCtx *>(lv_indev_get_user_data(indev));
    data->enc_diff = static_cast<int16_t>(ctx->encoder->consumeSteps());
    data->state = ctx->button->isPressed() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

struct ButtonCtx {
    PushButton *button;
    uint32_t key;
};
void buttonReadCb(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto *ctx = static_cast<ButtonCtx *>(lv_indev_get_user_data(indev));
    data->key = ctx->key;
    data->state = ctx->button->isPressed() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

}  // namespace

lv_display_t *createDisplay(PanelDriver &panel)
{
    lv_display_t *disp = lv_display_create(panel.width(), panel.height());
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(disp, &panel);
    lv_display_set_flush_cb(disp, flushCb);

    // Partial render buffers: a fraction of the full frame is plenty for a
    // small panel and keeps RAM use modest; LVGL flushes in that many rows
    // per chunk. Buffers must outlive this function (LVGL keeps the
    // pointers), hence `static`.
    const int rows = std::max(1, panel.height() / 10);
    const size_t bufBytes = static_cast<size_t>(panel.width()) * rows * sizeof(uint16_t);
    static std::vector<uint8_t> buf1, buf2;
    buf1.assign(bufBytes, 0);
    buf2.assign(bufBytes, 0);
    lv_display_set_buffers(disp, buf1.data(), buf2.data(), bufBytes,
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
    return disp;
}

lv_indev_t *createEncoderIndev(RotaryEncoder &encoder, PushButton &button, lv_group_t *group)
{
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, encoderReadCb);
    // Intentionally never freed: one indev per physical control, alive for
    // the process lifetime (conHMI has exactly one display/input set).
    lv_indev_set_user_data(indev, new EncoderCtx{&encoder, &button});
    lv_indev_set_group(indev, group);
    return indev;
}

lv_indev_t *createButtonIndev(PushButton &button, uint32_t key, lv_group_t *group)
{
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, buttonReadCb);
    lv_indev_set_user_data(indev, new ButtonCtx{&button, key});
    lv_indev_set_group(indev, group);
    return indev;
}

void runLoop(const std::atomic_bool &running, int periodMs, const std::function<void()> &onTick)
{
    while (running.load(std::memory_order_relaxed)) {
        lv_tick_inc(periodMs);
        lv_timer_handler();
        if (onTick) onTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs));
    }
}

} // namespace lvgl_glue
