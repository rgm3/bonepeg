#include <stdlib.h>

// converts an RGB pixel to luminosity / grey
// ITU-R Recommendation BT. 709: Y = 0.2126 R + 0.7152 G + 0.0722 B
uint8_t luma(uint8_t r, uint8_t g, uint8_t b) {
  float y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  return (uint8_t) y;
}
