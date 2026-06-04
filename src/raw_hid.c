#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usb_hid.h>
#include <string.h>
#include "raw_hid_display.h"

LOG_MODULE_REGISTER(raw_hid);

#define HID_USAGE_PAGE_VENDOR  0xFF00
#define HID_USAGE_VENDOR_DATA  0x01

static const uint8_t raw_hid_report_desc[] = {
  HID_ITEM(HID_ITEM_TAG_USAGE_PAGE, HID_ITEM_TYPE_GLOBAL, 2),
    0x00, 0xFF,
  HID_USAGE(HID_USAGE_VENDOR_DATA),
  HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_USAGE(HID_USAGE_VENDOR_DATA),
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX8(0xFF),
    HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(64),
    HID_INPUT(0x02),

    HID_USAGE(HID_USAGE_VENDOR_DATA),
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX8(0xFF),
    HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(64),
    HID_OUTPUT(0x02),
  HID_END_COLLECTION,
};

static const struct device *hid_dev;
static struct k_sem hid_sem;

static uint8_t rx_buf[64];
static int rx_offset;

void raw_hid_send(const uint8_t *data, uint32_t len)
{
  k_sem_take(&hid_sem, K_MSEC(30));
  hid_int_ep_write(hid_dev, data, len, NULL);
}

static void in_ready_cb(const struct device *dev)
{
  k_sem_give(&hid_sem);
}

static void out_ready_cb(const struct device *dev)
{
  uint8_t buf[64];
  uint32_t len;
  int ret = hid_int_ep_read(dev, buf, sizeof(buf), &len);
  if (ret != 0 || len == 0) return;

  if (rx_offset + len > sizeof(rx_buf)) {
    len = sizeof(rx_buf) - rx_offset;
  }
  memcpy(rx_buf + rx_offset, buf, len);
  rx_offset += len;

  if (rx_offset >= 64) {
    rx_offset = 0;
    LOG_INF("Received 64 bytes");
#if IS_ENABLED(CONFIG_ZMK_DISPLAY)
    raw_hid_display_process(rx_buf, 64);
#endif
  }
}

static const struct hid_ops ops = {
  .int_in_ready = in_ready_cb,
  .int_out_ready = out_ready_cb,
};

static int raw_hid_init(void)
{
  hid_dev = device_get_binding("HID_1");
  if (hid_dev == NULL) {
    LOG_ERR("Failed to get HID_1");
    return -EINVAL;
  }

  k_sem_init(&hid_sem, 1, 1);

  usb_hid_register_device(hid_dev, raw_hid_report_desc,
                          sizeof(raw_hid_report_desc), &ops);
  usb_hid_init(hid_dev);

  LOG_INF("Raw HID registered on HID_1");
  return 0;
}

SYS_INIT(raw_hid_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
