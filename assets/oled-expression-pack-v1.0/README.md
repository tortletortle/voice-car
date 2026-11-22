# Voice Car OLED Expression Pack V1.0

A native 128×64, 1-bit monochrome expression library for the voice-controlled car OLED.

## Contents

- `png/`: 11 static expressions and 12 animation frames (23 images total).
- `voice_car_oled_faces_rle_v1.h`: compact C99 header containing all 23 frames in PackBits-style RLE, with a decoder and per-frame CRC32 values.
- `manifest.json`: stable IDs, labels, and animation frame order.
- `validation.json`: generation and host-validation summary.
- `tests_test_oled_expression_rle.c`: host test for decompression and exact CRC32 matching.

## C usage

```c
#include "voice_car_oled_faces_rle_v1.h"

uint8_t framebuffer[VC_OLED_BITMAP_BYTES];
if (vc_oled_decode(VC_FACE_HAPPY, framebuffer, sizeof framebuffer)) {
    /* Send framebuffer to the existing 128×64 OLED drawing/flush routine. */
}
```

Decoded data uses SSD1306 page layout: byte index `(y / 8) * 128 + x`, bit `(y % 8)`, with 1 meaning lit pixel. Confirm the current OLED driver's framebuffer bit order before wiring this into the firmware.

## Host validation

```sh
cc -std=c99 -Wall -Wextra -Werror -pedantic tests_test_oled_expression_rle.c -o /tmp/test-oled
/tmp/test-oled
```

The test checks all 23 decoded frames against their stored CRC32. This validates the generated data and decoder, not the physical OLED panel. Driver integration and on-device visual acceptance are still pending.
