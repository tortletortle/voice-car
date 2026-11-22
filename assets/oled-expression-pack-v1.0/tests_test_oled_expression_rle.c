#include <stdio.h>
#include "voice_car_oled_faces_rle_v1.h"
static uint32_t crc32(const uint8_t *p, size_t n) {
  uint32_t c = 0xFFFFFFFFu;
  size_t i; unsigned b;
  for (i = 0; i < n; ++i) {
    c ^= p[i];
    for (b = 0; b < 8; ++b) c = (c >> 1) ^ ((c & 1u) ? 0xEDB88320u : 0u);
  }
  return c ^ 0xFFFFFFFFu;
}
int main(void) {
  uint8_t bitmap[VC_OLED_BITMAP_BYTES]; unsigned i;
  for (i = 0; i < VC_OLED_ASSET_COUNT; ++i) {
    if (!vc_oled_decode((vc_oled_asset_id_t)i, bitmap, sizeof bitmap)) return 10;
    if (crc32(bitmap, sizeof bitmap) != vc_oled_bitmap_crc32[i]) return 20 + (int)i;
  }
  puts("OLED RLE decode + CRC32: PASS (23/23 assets)");
  return 0;
}
