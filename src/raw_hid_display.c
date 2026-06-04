#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/wpm_status.h>

LOG_MODULE_REGISTER(raw_hid_display, CONFIG_ZMK_LOG_LEVEL);

#define MAX_LINE_LEN 63

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
static struct zmk_widget_battery_status battery_status_widget;
#endif
#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
static struct zmk_widget_output_status output_status_widget;
#endif
#if IS_ENABLED(CONFIG_ZMK_WIDGET_PERIPHERAL_STATUS)
static struct zmk_widget_peripheral_status peripheral_status_widget;
#endif
#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
static struct zmk_widget_layer_status layer_status_widget;
#endif
#if IS_ENABLED(CONFIG_ZMK_WIDGET_WPM_STATUS)
static struct zmk_widget_wpm_status wpm_status_widget;
#endif

static lv_obj_t *hid_label[2];
static lv_obj_t *widget_containers[5];
static int widget_count;

static lv_obj_t *canvas;
static uint8_t canvas_buf[512];
static uint8_t pixel_buf[512];

struct display_data {
    uint8_t cmd;
    bool showing_hid;
    bool showing_canvas;
    bool in_pixel_frame;
    char line0[MAX_LINE_LEN + 1];
    char line1[MAX_LINE_LEN + 1];
};

static struct display_data data;
static struct k_mutex data_mutex;

static void display_work_handler(struct k_work *work);

K_WORK_DEFINE(display_work, display_work_handler);

static void show_widgets(void)
{
    for (int i = 0; i < widget_count; i++) {
        lv_obj_clear_flag(widget_containers[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 2; i++) {
        lv_obj_add_flag(hid_label[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
}

static void show_hid(void)
{
    for (int i = 0; i < widget_count; i++) {
        lv_obj_add_flag(widget_containers[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 2; i++) {
        lv_obj_clear_flag(hid_label[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
}

static void show_canvas(void)
{
    for (int i = 0; i < widget_count; i++) {
        lv_obj_add_flag(widget_containers[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 2; i++) {
        lv_obj_add_flag(hid_label[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_HIDDEN);
}

static void convert_frame(void)
{
    memset(canvas_buf, 0, sizeof(canvas_buf));
    for (int page = 0; page < 4; page++) {
        for (int col = 0; col < 128; col++) {
            uint8_t ssd = pixel_buf[page * 128 + col];
            if (ssd == 0) continue;
            int dst_byte = col >> 3;
            int dst_bit = 7 - (col & 7);
            uint8_t mask = 1 << dst_bit;
            for (int b = 0; b < 8; b++) {
                if (ssd & (1 << b)) {
                    int row = page * 8 + b;
                    canvas_buf[row * 16 + dst_byte] |= mask;
                }
            }
        }
    }
}

static void display_work_handler(struct k_work *work)
{
    k_mutex_lock(&data_mutex, K_FOREVER);

    switch (data.cmd) {
    case 0x01:
        lv_label_set_text(hid_label[0], data.line0);
        if (!data.showing_hid) {
            data.showing_hid = true;
            show_hid();
        }
        break;
    case 0x02:
        lv_label_set_text(hid_label[1], data.line1);
        if (!data.showing_hid) {
            data.showing_hid = true;
            show_hid();
        }
        break;
    case 0x10:
        lv_label_set_text(hid_label[0], "");
        lv_label_set_text(hid_label[1], "");
        if (data.showing_canvas) {
            data.showing_canvas = false;
            show_widgets();
        } else if (data.showing_hid) {
            data.showing_hid = false;
            show_widgets();
        }
        break;
    case 0x21:
        convert_frame();
        if (!data.showing_canvas) {
            data.showing_canvas = true;
            data.showing_hid = false;
            show_canvas();
        }
        lv_obj_invalidate(canvas);
        break;
    case 0x22:
        memset(pixel_buf, 0, sizeof(pixel_buf));
        if (data.showing_canvas) {
            data.showing_canvas = false;
            show_widgets();
        }
        break;
    }

    data.cmd = 0;
    k_mutex_unlock(&data_mutex);
}

static void style_widget_container(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
}

lv_obj_t *zmk_display_status_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);

    widget_count = 0;

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_t *bat = zmk_widget_battery_status_obj(&battery_status_widget);
    style_widget_container(bat);
    widget_containers[widget_count++] = bat;
    lv_obj_align(bat, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_t *out = zmk_widget_output_status_obj(&output_status_widget);
    style_widget_container(out);
    widget_containers[widget_count++] = out;
    lv_obj_align(out, LV_ALIGN_TOP_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_PERIPHERAL_STATUS)
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_t *per = zmk_widget_peripheral_status_obj(&peripheral_status_widget);
    style_widget_container(per);
    widget_containers[widget_count++] = per;
    lv_obj_align(per, LV_ALIGN_TOP_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_t *lay = zmk_widget_layer_status_obj(&layer_status_widget);
    style_widget_container(lay);
    widget_containers[widget_count++] = lay;
    lv_obj_align(lay, LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_WPM_STATUS)
    zmk_widget_wpm_status_init(&wpm_status_widget, screen);
    lv_obj_t *wpm = zmk_widget_wpm_status_obj(&wpm_status_widget);
    style_widget_container(wpm);
    widget_containers[widget_count++] = wpm;
    lv_obj_align(wpm, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
#endif

    for (int i = 0; i < 2; i++) {
        hid_label[i] = lv_label_create(screen);
        lv_label_set_text(hid_label[i], "");
        lv_obj_set_style_text_align(hid_label[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(hid_label[i], lv_color_white(), 0);
        lv_obj_align(hid_label[i], i == 0 ? LV_ALIGN_TOP_MID : LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(hid_label[i], LV_OBJ_FLAG_HIDDEN);
    }

    canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(canvas, canvas_buf, 128, 32, LV_COLOR_FORMAT_I1);
    lv_obj_set_size(canvas, 128, 32);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);

    lv_canvas_set_palette(canvas, 0, (lv_color32_t){.red = 0, .green = 0, .blue = 0, .alpha = 0xFF});
    lv_canvas_set_palette(canvas, 1, (lv_color32_t){.red = 0xFF, .green = 0xFF, .blue = 0xFF, .alpha = 0xFF});

    return screen;
}

void raw_hid_display_process(const uint8_t *buf, uint32_t len)
{
    if (!zmk_display_is_initialized() || len < 1) {
        return;
    }

    k_mutex_lock(&data_mutex, K_FOREVER);

    uint32_t copy_len = len - 1;
    uint8_t cmd = buf[0];
    bool needs_work = false;

    if (cmd == 0x00 && !data.in_pixel_frame) {
        data.in_pixel_frame = true;
        if (copy_len > 63) copy_len = 63;
        memcpy(pixel_buf + 1, buf + 1, copy_len);
    } else if (data.in_pixel_frame && cmd < 8) {
        if (copy_len > 63) copy_len = 63;
        memcpy(pixel_buf + cmd * 64 + 1, buf + 1, copy_len);
    } else if (cmd == 0xFF && data.in_pixel_frame) {
        data.in_pixel_frame = false;
        for (int i = 0; i < 8 && i < copy_len; i++) {
            pixel_buf[i * 64] = buf[1 + i];
        }
        data.cmd = 0x21;
        needs_work = true;
    } else if (cmd == 0x01) {
        data.in_pixel_frame = false;
        char *target = data.line0;
        if (copy_len > MAX_LINE_LEN) copy_len = MAX_LINE_LEN;
        memcpy(target, buf + 1, copy_len);
        target[copy_len] = '\0';
        data.cmd = cmd;
        needs_work = true;
    } else if (cmd == 0x02) {
        data.in_pixel_frame = false;
        char *target = data.line1;
        if (copy_len > MAX_LINE_LEN) copy_len = MAX_LINE_LEN;
        memcpy(target, buf + 1, copy_len);
        target[copy_len] = '\0';
        data.cmd = cmd;
        needs_work = true;
    } else if (cmd == 0x10) {
        data.in_pixel_frame = false;
        memset(pixel_buf, 0, sizeof(pixel_buf));
        data.cmd = cmd;
        needs_work = true;
    }

    k_mutex_unlock(&data_mutex);

    if (needs_work) {
        k_work_submit_to_queue(zmk_display_work_q(), &display_work);
    }
}
