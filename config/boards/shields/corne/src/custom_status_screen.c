/*
 * Custom Corne peripheral (right-half) status screen.
 *
 * Battery + connection widgets on top, a self-running equalizer animation
 * (16 bars) along the bottom. The bars are real LVGL objects whose height is
 * updated on a local LVGL timer — changing an object's geometry always marks
 * it dirty, so every frame is guaranteed to redraw (no image decode/cache in
 * the path). The peripheral has no layer/WPM/key state, so the animation is
 * fully self-contained: it needs nothing relayed from the central half.
 *
 * White (lit) bars on a black (OFF) background keep the lit-pixel count low,
 * which is friendly to the peripheral battery and BLE link.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <lvgl.h>

#define EQ_BARS    16
#define EQ_PITCH   8            /* px between bar starts (16 * 8 = 128 wide)  */
#define EQ_BAR_W   6            /* lit width, leaving a 2px gap               */
#define EQ_MAXH    16           /* tallest bar (lower half of a 32px panel)   */
#define EQ_BASE_Y  32           /* bottom edge of the panel                   */
#define EQ_FPS_MS  60           /* ~16 fps                                    */

/* sin(2*pi*k/64) scaled to 0..255 */
static const uint8_t sine64[64] = {
    128, 140, 152, 165, 176, 188, 198, 208,
    218, 226, 234, 240, 245, 250, 253, 254,
    255, 254, 253, 250, 245, 240, 234, 226,
    218, 208, 198, 188, 176, 165, 152, 140,
    128, 115, 103,  90,  79,  67,  57,  47,
     37,  29,  21,  15,  10,   5,   2,   1,
      0,   1,   2,   5,  10,  15,  21,  29,
     37,  47,  57,  67,  79,  90, 103, 115,
};

static lv_obj_t *bars[EQ_BARS];
static lv_style_t bar_style;
static uint16_t eq_phase;

static void eq_timer_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    eq_phase++;
    for (int b = 0; b < EQ_BARS; b++) {
        /* each bar bounces at its own rate + phase for a lively look */
        unsigned idx = (eq_phase * (2 + (b & 3)) + b * 11) & 63;
        int h = 1 + (int)(sine64[idx] * (EQ_MAXH - 1)) / 255;   /* 1..EQ_MAXH */
        lv_obj_set_pos(bars[b], b * EQ_PITCH + 1, EQ_BASE_Y - h);
        lv_obj_set_height(bars[b], h);
    }
}

/* ── Screen layout (128x32 OLED) ─────────────────────────────────────────
 *  top-left : connection (peripheral status)   top-right : battery
 *  bottom   : equalizer animation (full width)
 */

static struct zmk_widget_battery_status battery_widget;
static struct zmk_widget_peripheral_status peripheral_widget;

lv_obj_t *zmk_display_status_screen(void) {
    /* Plain themed screen (like ZMK's built-in): the mono theme supplies the
     * dark background and the inherited text color the label widgets need.
     * Don't strip it, or the battery/connection labels render dark-on-dark.
     * Only zero the padding (so the bars' absolute coords span the full 128px)
     * and disable scrolling. */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* connection — top left */
    zmk_widget_peripheral_status_init(&peripheral_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_widget),
                 LV_ALIGN_TOP_LEFT, 0, 0);

    /* battery — top right */
    zmk_widget_battery_status_init(&battery_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_widget),
                 LV_ALIGN_TOP_RIGHT, 0, 0);

    /* equalizer — lit rectangles anchored to the bottom edge */
    lv_style_init(&bar_style);
    lv_style_set_bg_opa(&bar_style, LV_OPA_COVER);
    lv_style_set_bg_color(&bar_style, lv_color_white());
    lv_style_set_border_width(&bar_style, 0);
    lv_style_set_radius(&bar_style, 0);
    lv_style_set_pad_all(&bar_style, 0);

    for (int b = 0; b < EQ_BARS; b++) {
        bars[b] = lv_obj_create(screen);
        lv_obj_remove_style_all(bars[b]);
        lv_obj_add_style(bars[b], &bar_style, 0);
        lv_obj_clear_flag(bars[b], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_width(bars[b], EQ_BAR_W);
        lv_obj_set_height(bars[b], 1);
        lv_obj_set_pos(bars[b], b * EQ_PITCH + 1, EQ_BASE_Y - 1);
    }

    lv_timer_create(eq_timer_cb, EQ_FPS_MS, NULL);

    return screen;
}
