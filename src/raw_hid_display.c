// SPDX-License-Identifier: MIT
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "raw_hid_display.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void draw_test_pixel(void) {
    /* Create a tiny object (1x1 pixel) at a fixed position */
    lv_obj_t *pixel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pixel, 1, 1);
    lv_obj_set_style_bg_color(pixel, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pixel, LV_OPA_COVER, LV_PART_MAIN);
    /* Position at (10,10) – adjust as needed */
    lv_obj_set_pos(pixel, 10, 10);
}

void raw_hid_display_process(const uint8_t *data, size_t len)
{
    LOG_DBG("raw_hid_display_process called, len=%zu", len);
    /* For now we simply draw a pixel regardless of input data */
    draw_test_pixel();
    /* In a real implementation you would parse `data` and update the LVGL
     * canvas or widgets accordingly. */
    (void)data; // suppress unused warning
}
