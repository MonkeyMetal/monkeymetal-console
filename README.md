# gbemu

GameBoy / GameBoy Color emulator running on the
[Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2)
(4.2" reflective-LCD AIoT board, ST7305 1bpp 300x400 panel).

Status: **work in progress**. Hardware bring-up + LCD/SD/WiFi/BOOT key test
all verified. gnuboy CPU emulation core links, loads the ROM, then
crashes on the first instruction (`rom.bank` pointer corruption — bug
under investigation).

## Hardware

- Waveshare ESP32-S3-RLCD-4.2 (ESP32-S3, 8 MB Octal PSRAM, 16 MB flash, ST7305)
- TF/SD card slot (1-bit SDMMC) holds the ROM as `/sdcard/rom.gbc`
- USB-C for power + flashing + USB-JTAG (no programmer needed)

## Build

Requires ESP-IDF v5.5.x. With the IDF environment active:

```bash
cd gbemu
idf.py set-target esp32s3
idf.py build flash monitor
```

Or in VS Code with the Espressif IDF extension: `Ctrl+Shift+P` →
`ESP-IDF: Build, Flash and Monitor your Project`.

The first build is slow (LVGL is **not** used, but Wi-Fi / lwIP take time).

## WiFi credentials

Copy `components/wifi_bsp/user_secrets.h.example` to
`components/wifi_bsp/user_secrets.h` and fill in your SSID / password.
The real `user_secrets.h` is gitignored.

## Project layout

```
gbemu/
├── main/                          app entry, boot splash, key test mode
├── components/
│   ├── port_bsp/                  ST7305 LCD + SDMMC drivers (from Waveshare demo)
│   ├── wifi_bsp/                  WiFi STA bring-up
│   ├── gnuboy/                    gnuboy 1.0.4 emulator core (GPLv2)
│   └── gnuboy_sys_esp32idf/       platform port: vid_/pcm_/ev_/sys_* hooks
├── partitions.csv                 8 MB factory + 4 MB FAT storage
└── sdkconfig.defaults             PSRAM Octal @80MHz, BSS to PSRAM, etc.
```

## Roadmap

- [x] LCD bring-up (ST7305 1bpp via SPI3)
- [x] SD card mount, ROM file load
- [x] WiFi STA connect
- [x] gnuboy core + sys port skeleton (links, runs `loader_init`, prints ROM title)
- [x] Boot splash ("MonkeyMetal") + reset-reason / BOOT-key diagnostic
- [ ] Fix `rom.bank` corruption so `cpu_emulate` runs
- [ ] Bayer 4x4 dither: 16bpp intermediate FB → 1bpp ST7305
- [ ] WiFi UDP keypad service (PC keyboard as game controller)
- [ ] Hardware 8-key board (DPad + AB + START/SELECT)
- [ ] ES8311 audio output

## License

GPL-2.0-or-later. The bulk of the codebase comes from
[gnuboy 1.0.4](http://gnuboy.unix-fu.org/) which is GPLv2 — see `LICENSE`.
The Waveshare demo drivers in `components/port_bsp/` are reused under the
same terms unless their original headers state otherwise.
```

You are welcome to use, modify, and redistribute the code under GPLv2,
but **you are responsible for providing your own ROMs**. No copyrighted
ROM is included or linked from this repository.

## Acknowledgements

- [gnuboy](http://gnuboy.unix-fu.org/) — Laguna, et al.
- [Waveshare](https://www.waveshare.net/) — ESP32-S3-RLCD-4.2 demo code
- [Espressif](https://www.espressif.com/) — ESP-IDF toolchain
