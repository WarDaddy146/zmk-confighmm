/*
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Canvas size matches the SSD1306 128x32 display */
#define CANVAS_W 128
#define CANVAS_H 32
#define CANVAS_COLOR_FORMAT LV_COLOR_FORMAT_I1
/* Calculate buffer size with alignment */
#define CANVAS_BUF_SIZE \
    LV_CANVAS_BUF_SIZE(CANVAS_W, CANVAS_H, LV_COLOR_FORMAT_GET_BPP(CANVAS_COLOR_FORMAT), LV_DRAW_BUF_STRIDE_ALIGN)
static uint8_t canvas_buf[CANVAS_BUF_SIZE];

lv_obj_t *zmk_display_status_screen(void)
{
    /* Root screen object */
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Create a canvas attached to the screen */
    lv_obj_t *canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_W, CANVAS_H, CANVAS_COLOR_FORMAT);

    /* Initialise palette for 1‑bit colour */
    lv_canvas_set_palette(canvas, 0, lv_color_to_32(lv_color_black(), LV_OPA_COVER));
    lv_canvas_set_palette(canvas, 1, lv_color_to_32(lv_color_white(), LV_OPA_COVER));

    /* Clear to black */
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    /* Light a single pixel at (10,10) – use index 1 (white) */
    /* For LV_COLOR_DEPTH_1 the colour index is encoded in the blue channel */
    lv_canvas_set_px(canvas, 10, 10, lv_color_make(0, 0, 1), LV_OPA_COVER);

    return screen;
}
