idf_component_register(
    SRCS
        "bt_hid_host.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        bt
    PRIV_REQUIRES
        freertos
)

# ESP-IDF 5.x BT component has some API headers in paths that
# aren't always exposed as PUBLIC. Add them manually.
# - host/bluedroid/api/include/api: esp_gap_ble_api.h, esp_gattc_api.h, etc.
# - include/esp32c3/include: esp_bt.h (ESP32-S3 reuses ESP32C3 BT controller)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${IDF_PATH}/components/bt/host/bluedroid/api/include/api"
    "${IDF_PATH}/components/bt/include/esp32c3/include"
)
