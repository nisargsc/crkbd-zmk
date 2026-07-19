/*
 * Custom Corne peripheral (right-half) status screen.
 *
 * A self-running equalizer animation plus the built-in battery + connection
 * widgets. The animation is driven by a local LVGL timer, so it needs no
 * state relayed from the central half — the peripheral has no layer/WPM/key
 * state of its own. Lit bars on a dark background keep the lit-pixel count
 * low, which is friendly to the peripheral battery and BLE link.
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <lvgl.h>

/* ── Equalizer animation ─────────────────────────────────────────────────
 * Rendered into a 1-bpp (LV_COLOR_FORMAT_I1) framebuffer that is re-shown on
 * an LVGL timer. The buffer is [8-byte palette][bitmap]; palette index 0 is
 * black (background, OFF) and index 1 is white (bars, LIT).
 */

#define EQ_W       128
#define EQ_H       16
#define EQ_STRIDE  (EQ_W / 8)          /* 16 bytes per row            */
#define EQ_BARS    16
#define EQ_PITCH   (EQ_W / EQ_BARS)    /* 8 px per bar                */
#define EQ_BAR_W   6                   /* 6 px lit, 2 px gap          */
#define EQ_FPS_MS  60                  /* ~16 fps                     */

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

static uint8_t eq_buf[8 + EQ_STRIDE * EQ_H];

static lv_image_dsc_t eq_dsc = {
    .header = { .cf = LV_COLOR_FORMAT_I1, .w = EQ_W, .h = EQ_H },
    .data_size = sizeof(eq_buf),
    .data = eq_buf,
};

static lv_obj_t *eq_img;
static uint16_t eq_phase;

static inline void eq_set_px(uint8_t *px, int x, int y) {
    px[y * EQ_STRIDE + (x >> 3)] |= (0x80 >> (x & 7));
}

static void eq_render(void) {
    /* palette: 0 = black (OFF, background), 1 = white (LIT, bars) */
    eq_buf[0] = 0x00; eq_buf[1] = 0x00; eq_buf[2] = 0x00; eq_buf[3] = 0xFF;
    eq_buf[4] = 0xFF; eq_buf[5] = 0xFF; eq_buf[6] = 0xFF; eq_buf[7] = 0xFF;

    uint8_t *px = eq_buf + 8;
    memset(px, 0, EQ_STRIDE * EQ_H);

    for (int b = 0; b < EQ_BARS; b++) {
        /* each bar bounces at its own rate + phase for a lively look */
        unsigned idx = (eq_phase * (2 + (b & 3)) + b * 11) & 63;
        unsigned level = sine64[idx];
        int h = 1 + (int)(level * (EQ_H - 1)) / 255;   /* 1..EQ_H */

        int x0 = b * EQ_PITCH + 1;
        for (int x = x0; x < x0 + EQ_BAR_W && x < EQ_W; x++) {
            for (int y = EQ_H - h; y < EQ_H; y++) {
                eq_set_px(px, x, y);
            }
        }
    }

    lv_image_set_src(eq_img, &eq_dsc);
    lv_obj_invalidate(eq_img);
}

static void eq_timer_cb(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    eq_phase++;
    eq_render();
}

/* ── Screen layout (128x32 OLED) ─────────────────────────────────────────
 *  top-left : connection (peripheral status)   top-right : battery
 *  bottom   : equalizer animation (full width)
 */

static struct zmk_widget_battery_status battery_widget;
static struct zmk_widget_peripheral_status peripheral_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* connection — top left */
    zmk_widget_peripheral_status_init(&peripheral_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_widget),
                 LV_ALIGN_TOP_LEFT, 0, 0);

    /* battery — top right */
    zmk_widget_battery_status_init(&battery_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_widget),
                 LV_ALIGN_TOP_RIGHT, 0, 0);

    /* equalizer — bottom, full width */
    eq_img = lv_image_create(screen);
    lv_obj_align(eq_img, LV_ALIGN_BOTTOM_MID, 0, 0);
    eq_render();
    lv_timer_create(eq_timer_cb, EQ_FPS_MS, NULL);

    return screen;
}
