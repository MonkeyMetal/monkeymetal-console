idf_component_register(
    SRCS
        # MonkeyMetal audio engine wrapper
        "audio_engine.c"
        # esp_codec_dev library
        "esp_codec_dev/audio_codec_sw_vol.c"
        "esp_codec_dev/esp_codec_dev.c"
        "esp_codec_dev/esp_codec_dev_if.c"
        "esp_codec_dev/esp_codec_dev_vol.c"
        "esp_codec_dev/platform/audio_codec_ctrl_i2c.c"
        "esp_codec_dev/platform/audio_codec_ctrl_spi.c"
        "esp_codec_dev/platform/audio_codec_data_i2s.c"
        "esp_codec_dev/platform/audio_codec_gpio.c"
        "esp_codec_dev/platform/esp_codec_dev_os.c"
        "esp_codec_dev/device/es8311/es8311.c"
        "esp_codec_dev/device/es7210/es7210.c"
        # codec_board (board config parser + init)
        "codec_board/cfg_parse.c"
        "codec_board/codec_board.c"
        "codec_board/codec_init.c"
        "codec_board/dummy_codec.c"
        "codec_board/drv/tca9554.c"
    INCLUDE_DIRS
        "."
        "esp_codec_dev/include"
        "esp_codec_dev/interface"
        "esp_codec_dev/device/include"
        "esp_codec_dev/device/priv_include"
        "codec_board"
        "codec_board/include"
        "codec_board/drv"
    REQUIRES
        driver
        esp_driver_i2s
        esp_driver_i2c
    PRIV_REQUIRES
        esp_timer
        freertos
)

# Suppress warnings from vendor code
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-missing-field-initializers
    -Wno-implicit-fallthrough
    -Wno-deprecated-declarations
)
