if(CONFIG_RAW_HID)
  zephyr_library()
  zephyr_library_sources(src/raw_hid.c)
endif()

if(CONFIG_RAW_HID AND CONFIG_ZMK_DISPLAY)
  target_sources(app PRIVATE src/raw_hid_display.c)
endif()
