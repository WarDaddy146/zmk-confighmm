// Built against LVGL v9.x — native 1-bit pixel object
#include <lvgl.h>
#include <zephyr/kernel.h>

lv_obj_t *zmk_display_status_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *pixel = lv_obj_create(screen);
    lv_obj_set_size(pixel, 1, 1);
    lv_obj_set_pos(pixel, 0, 0);
    lv_obj_set_style_bg_color(pixel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pixel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(pixel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pixel, 0, LV_PART_MAIN);

    return screen;
}
