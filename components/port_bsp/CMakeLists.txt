idf_component_register(
    SRCS
        "display_bsp.cpp"
        "sdcard_bsp.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        # 这些被 .h 头文件直接 include, 必须暴露给依赖者
        driver
        esp_lcd
        esp_driver_sdmmc
        fatfs
    PRIV_REQUIRES
        esp_timer
)
