/*
 * Color metrics.
 */

#include <stdlib.h>


/**
 * Finds the luma (Y') (percieved brightness / grey level) value for a gamma-corrected RGB pixel.
 * In the case that the pixel is not gamma corrected, this value is the luminance (Y).
 *
 * ITU-R Recommendation BT. 709: Y = 0.2126 R + 0.7152 G + 0.0722 B
 *
 * Consider using OpenCV's cvtColor per-frame instead of this per-pixel function.
 *   - cv::cvtColor(colorMat, greyMat, cv::COLOR_BGR2GRAY);
 *   - http://docs.opencv.org/modules/imgproc/doc/miscellaneous_transformations.html#cvtcolor
 */
uint8_t luma(uint8_t r, uint8_t g, uint8_t b) {
  float y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  return (uint8_t) y;
}

/**
 * Returns the maximum of the three arguments.
 */
int max3(int a, int b, int c) {
  if (a > b && a > c ) { return a; }
  if (b > a && b > c ) { return b; }
  return c;
}

/**
 * Returns the minimum of the three arguments.
 */
int min3(int a, int b, int c) {
  if (a < b && a < c ) { return a; }
  if (b < a && b < c ) { return b; }
  return c;
}

/**
 * Find saturation of an RGB pixel as value in range [0,255].
 *
 * Saturation is the colorfulness of a color relative to its own brightness.
 * Low saturation images are closer to greyscale.
 *
 * See:
 *   - http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
 *   - http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
 *
 * @param r the red color component
 * @param g the green color component
 * @param b the blue color component
 * @return saturation value between 0 and 255 inclusive, corresponding to 0 to 100%
 */
uint8_t saturation(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t min, max, delta, s;

  min = min3(r, g, b);
  max = max3(r, g, b);

  if (max != 0) {
    delta = max - min;
    s = long(delta) * 255 / max;
  } else {
    s = 0;
  }

  return s;
}

