// Built against LVGL v9.x — LV_COLOR_FORMAT_I1 format
#include <lvgl.h>
#include <zephyr/kernel.h>

static uint8_t canvas_buf[128 * 32 / 8];

lv_obj_t *zmk_display_status_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(canvas, canvas_buf, 128, 32, LV_COLOR_FORMAT_I1);

    lv_canvas_set_palette(canvas, 0, lv_color_to_32(lv_color_black(), LV_OPA_COVER));
    lv_canvas_set_palette(canvas, 1, lv_color_to_32(lv_color_white(), LV_OPA_COVER));

    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    lv_canvas_set_px(canvas, 0, 0, lv_color_make(0, 0, 1), LV_OPA_COVER);

    return screen;
}
