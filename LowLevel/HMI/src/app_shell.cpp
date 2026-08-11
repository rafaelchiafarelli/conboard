#include "app_shell.hpp"

#include <algorithm>

namespace appshell {

Shell::Shell(lv_group_t *group) : group_(group) {}

lv_obj_t *Shell::pushScreen()
{
    lv_obj_t *scr = lv_obj_create(nullptr);
    stack_.push_back(scr);
    lv_screen_load(scr);
    return scr;
}

void Shell::popScreen()
{
    if (stack_.size() <= 1)
        return;
    lv_obj_t *top = stack_.back();
    stack_.pop_back();
    lv_screen_load(stack_.back());
    lv_obj_delete(top);
}

namespace {
struct MenuItemCtx {
    std::function<void()> onActivate;
};

void menuItemEventCb(lv_event_t *e)
{
    auto *ctx = static_cast<MenuItemCtx *>(lv_event_get_user_data(e));
    if (ctx && ctx->onActivate)
        ctx->onActivate();
}
}  // namespace

lv_obj_t *createMenuList(Shell & /*shell*/, lv_obj_t *parent)
{
    lv_obj_t *list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    return list;
}

lv_obj_t *addMenuItem(Shell &shell, lv_obj_t *list, const std::string &text,
                       std::function<void()> onActivate)
{
    lv_obj_t *btn = lv_list_add_button(list, nullptr, text.c_str());
    lv_group_add_obj(shell.group(), btn);

    // The context and its callback outlive the button for the button's own
    // lifetime -- LVGL deletes user_data-holding event descriptors along
    // with the object, but the std::function itself needs its own storage.
    auto *ctx = new MenuItemCtx{std::move(onActivate)};
    lv_obj_add_event_cb(btn, menuItemEventCb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e) { delete static_cast<MenuItemCtx *>(lv_event_get_user_data(e)); },
        LV_EVENT_DELETE, ctx);

    return btn;
}

ValueRow::ValueRow(lv_obj_t *parent, const std::string &label, int initial,
                    int minVal, int maxVal, std::function<void(int)> onChange)
    : value_(initial), minVal_(minVal), maxVal_(maxVal), onChange_(std::move(onChange))
{
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER);

    lv_obj_t *nameLabel = lv_label_create(container_);
    lv_label_set_text(nameLabel, label.c_str());

    valueLabel_ = lv_label_create(container_);
    refreshLabel();
}

void ValueRow::adjust(int delta)
{
    int next = std::clamp(value_ + delta, minVal_, maxVal_);
    if (next == value_)
        return;
    value_ = next;
    refreshLabel();
    if (onChange_)
        onChange_(value_);
}

void ValueRow::refreshLabel()
{
    lv_label_set_text_fmt(valueLabel_, "%d", value_);
}

lv_obj_t *createInfoLabel(lv_obj_t *parent, const std::string &text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text.c_str());
    lv_obj_set_width(label, lv_pct(90));
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_center(label);
    return label;
}

} // namespace appshell
