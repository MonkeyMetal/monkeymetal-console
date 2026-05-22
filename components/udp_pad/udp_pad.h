/*
 * udp_pad.h - WiFi UDP keypad service.
 *
 * Listens on UDP port 8888 for 2-byte packets:
 *   byte 0 = button id (UDP_PAD_*)
 *   byte 1 = action     (0 = release, 1 = press)
 *
 * Each packet is forwarded to one of:
 *   - the gnuboy input queue (when emulator is running)
 *   - a user callback for diagnostic UIs (e.g. key_test mode)
 *
 * Usage:
 *   udp_pad_init();                          // start UDP listener task
 *   udp_pad_set_callback(my_cb, my_arg);     // optional, for testing
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Button IDs sent over the wire and matching Python pad script. */
#define UDP_PAD_UP      1
#define UDP_PAD_DOWN    2
#define UDP_PAD_LEFT    3
#define UDP_PAD_RIGHT   4
#define UDP_PAD_A       5
#define UDP_PAD_B       6
#define UDP_PAD_START   7
#define UDP_PAD_SELECT  8
#define UDP_PAD_MAX     8

/* Optional test hook: invoked from the UDP task on every valid packet.
 * Pass NULL to clear. The hook runs in the UDP task context. */
typedef void (*udp_pad_cb_t)(uint8_t btn_id, bool pressed, void *arg);

void udp_pad_init(void);
void udp_pad_set_callback(udp_pad_cb_t cb, void *arg);

/* Friendly button name for logs / on-screen labels. */
const char *udp_pad_btn_name(uint8_t btn_id);

#ifdef __cplusplus
}
#endif
