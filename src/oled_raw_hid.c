#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <raw_hid/events.h>
#include <zmk/event_manager.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define OLED_ADDR 0x3C
#define FB_SIZE 512

#define I2C_NODE DT_NODELABEL(i2c1)

static const struct device *i2c_dev;
static uint8_t framebuffer[FB_SIZE];

static void oled_cmd(uint8_t c)
{
    uint8_t buf[2] = {0x00, c};
    i2c_write(i2c_dev, buf, sizeof(buf), OLED_ADDR);
}

static void oled_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x1F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x02,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) {
        oled_cmd(init_seq[i]);
    }

    oled_cmd(0x21);
    oled_cmd(0);
    oled_cmd(127);

    oled_cmd(0x22);
    oled_cmd(0);
    oled_cmd(3);
}

static void oled_flush(void)
{
    for (int offset = 0; offset < FB_SIZE; offset += 32) {
        uint8_t buf[33] = {0x40};
        memcpy(&buf[1], &framebuffer[offset], 32);
        i2c_write(i2c_dev, buf, sizeof(buf), OLED_ADDR);
    }
}

static int raw_hid_listener(const zmk_event_t *eh)
{
    struct raw_hid_received_event *ev = as_raw_hid_received_event(eh);
    if (!ev || ev->length != 64) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    uint8_t chunk = ev->data[0];

    if (chunk < 8) {
        memcpy(&framebuffer[chunk * 64 + 1], &ev->data[1], 63);
    } else if (chunk == 0xFF) {
        for (int i = 0; i < 8; i++) {
            framebuffer[i * 64] = ev->data[1 + i];
        }
        oled_flush();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(heilmary_oled, raw_hid_listener);
ZMK_SUBSCRIPTION(heilmary_oled, raw_hid_received_event);

static int oled_init_device(void)
{
    i2c_dev = DEVICE_DT_GET(I2C_NODE);
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    oled_init();
    LOG_INF("OLED initialized");
    return 0;
}

SYS_INIT(oled_init_device, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
