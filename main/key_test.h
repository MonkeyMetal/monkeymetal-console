#pragma once

#include "display_bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Key-test mode: shows last reset reason, then a live BOOT-button indicator.
 * Never returns. Replaces gnuboy_sys_run while we debug input. */
void key_test_run(DisplayPort *lcd);

#ifdef __cplusplus
}
#endif
