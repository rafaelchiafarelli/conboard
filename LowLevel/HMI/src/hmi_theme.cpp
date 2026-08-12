#include "hmi_theme.hpp"

namespace hmi_theme {
namespace {

// 1:1 with frontend/console/src/index.css :root.
constexpr uint32_t kGround     = 0x0f1216;
constexpr uint32_t kPanel      = 0x191d24;
constexpr uint32_t kPanel2     = 0x21262f;
constexpr uint32_t kRaise      = 0x272d38;
constexpr uint32_t kLine       = 0x2c323c;
constexpr uint32_t kInk        = 0xccd2db;
constexpr uint32_t kAccent     = 0xf2a541;
constexpr uint32_t kAccentSoft = 0x2a2214;
constexpr uint32_t kAccentDim  = 0x8a6a37;

struct Styles {
    lv_style_t root;               // screens (Shell::pushScreen())
    lv_style_t panel;               // plain containers (e.g. ValueRow's row)
    lv_style_t list;
    lv_style_t listButton;
    lv_style_t listButtonFocused;   // encoder focus / "confirm" target
    lv_style_t listButtonPressed;
    lv_style_t scrollbar;
    bool inited = false;
};

Styles &styles()
{
    static Styles s;
    return s;
}

void initStyles()
{
    Styles &s = styles();
    if (s.inited)
        return;
    s.inited = true;

    lv_style_init(&s.root);
    lv_style_set_bg_color(&s.root, lv_color_hex(kGround));
    lv_style_set_bg_opa(&s.root, LV_OPA_COVER);
    lv_style_set_text_color(&s.root, lv_color_hex(kInk));
    lv_style_set_border_width(&s.root, 0);

    lv_style_init(&s.panel);
    lv_style_set_bg_color(&s.panel, lv_color_hex(kPanel2));
    lv_style_set_bg_opa(&s.panel, LV_OPA_COVER);
    lv_style_set_border_color(&s.panel, lv_color_hex(kLine));
    lv_style_set_border_width(&s.panel, 1);
    lv_style_set_radius(&s.panel, 6);
    lv_style_set_pad_all(&s.panel, 8);

    lv_style_init(&s.list);
    lv_style_set_bg_color(&s.list, lv_color_hex(kPanel));
    lv_style_set_bg_opa(&s.list, LV_OPA_COVER);
    lv_style_set_border_width(&s.list, 0);
    lv_style_set_radius(&s.list, 0);
    lv_style_set_pad_row(&s.list, 0);

    lv_style_init(&s.listButton);
    lv_style_set_bg_opa(&s.listButton, LV_OPA_TRANSP);
    lv_style_set_text_color(&s.listButton, lv_color_hex(kInk));
    lv_style_set_border_side(&s.listButton, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_color(&s.listButton, lv_color_hex(kLine));
    lv_style_set_border_width(&s.listButton, 1);
    lv_style_set_radius(&s.listButton, 0);
    lv_style_set_pad_all(&s.listButton, 10);

    // Encoder-group focus == LVGL's LV_STATE_FOCUS_KEY (group navigation);
    // LV_STATE_FOCUSED covers a direct focus (e.g. touch, if ever added).
    // Mirrors the console's `.viewnav button.on` accent treatment.
    lv_style_init(&s.listButtonFocused);
    lv_style_set_bg_color(&s.listButtonFocused, lv_color_hex(kAccentSoft));
    lv_style_set_bg_opa(&s.listButtonFocused, LV_OPA_COVER);
    lv_style_set_text_color(&s.listButtonFocused, lv_color_hex(kAccent));
    lv_style_set_border_side(&s.listButtonFocused, LV_BORDER_SIDE_LEFT);
    lv_style_set_border_color(&s.listButtonFocused, lv_color_hex(kAccent));
    lv_style_set_border_width(&s.listButtonFocused, 3);

    lv_style_init(&s.listButtonPressed);
    lv_style_set_bg_color(&s.listButtonPressed, lv_color_hex(kRaise));
    lv_style_set_bg_opa(&s.listButtonPressed, LV_OPA_COVER);

    lv_style_init(&s.scrollbar);
    lv_style_set_bg_color(&s.scrollbar, lv_color_hex(kAccentDim));
    lv_style_set_bg_opa(&s.scrollbar, LV_OPA_COVER);
    lv_style_set_width(&s.scrollbar, 3);
}

void themeApply(lv_theme_t *, lv_obj_t *obj)
{
    Styles &s = styles();
    lv_obj_t *parent = lv_obj_get_parent(obj);

    if (!parent) {
        // A screen -- Shell::pushScreen()'s lv_obj_create(nullptr).
        lv_obj_add_style(obj, &s.root, 0);
        lv_obj_add_style(obj, &s.scrollbar, LV_PART_SCROLLBAR);
        return;
    }

    if (lv_obj_check_type(obj, &lv_list_class)) {
        lv_obj_add_style(obj, &s.list, 0);
        lv_obj_add_style(obj, &s.scrollbar, LV_PART_SCROLLBAR);
        return;
    }
    if (lv_obj_check_type(obj, &lv_list_button_class)) {
        lv_obj_add_style(obj, &s.listButton, 0);
        lv_obj_add_style(obj, &s.listButtonFocused, LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s.listButtonFocused, LV_STATE_FOCUS_KEY);
        lv_obj_add_style(obj, &s.listButtonPressed, LV_STATE_PRESSED);
        return;
    }
    if (lv_obj_check_type(obj, &lv_obj_class)) {
        // A plain container: ValueRow's row, or any future card-style group.
        lv_obj_add_style(obj, &s.panel, 0);
    }
    // Labels (and anything else) inherit root's text_color -- LVGL treats
    // text_color as an inherited style property, no per-label rule needed.
}

}  // namespace

void init(lv_display_t *disp)
{
    initStyles();

    static lv_theme_t *theme = nullptr;
    if (!theme) {
        theme = lv_theme_create();
        lv_theme_set_apply_cb(theme, themeApply);
    }
    lv_display_set_theme(disp, theme);
}

}  // namespace hmi_theme
