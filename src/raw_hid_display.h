// SPDX-License-Identifier: MIT
#ifndef RAW_HID_DISPLAY_H
#define RAW_HID_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

/**
 * Process raw HID data and update the display.
 *
 * This simple implementation ignores the incoming data and just draws a
 * single pixel on the SSD1306 display using LVGL.
 */
void raw_hid_display_process(const uint8_t *data, size_t len);

#endif // RAW_HID_DISPLAY_H
