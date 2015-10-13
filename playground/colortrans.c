/*
 * Takes an 8-bit RGB color in hexadecimal notation and finds the
 * nearest color and index in the default xterm 256 pallete.
 *
 * Ported from colortrans.py by Micah Elliott.
 * Original python code copyright Micah Elliott 2011.
 *
 * C version by rgm, April 2015
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct rgb { uint8_t r; uint8_t g; uint8_t b; };

// from linux vt.c
struct rgb vt_rgb_from_256(int i)
{
        struct rgb c;
        uint8_t scale = 85; // 102 is closer
        if (i < 8) {            /* Standard colours. */
                c.r = i&1 ? 0xaa : 0x00;
                c.g = i&2 ? 0xaa : 0x00;
                c.b = i&4 ? 0xaa : 0x00;
        } else if (i < 16) {
                c.r = i&1 ? 0xff : 0x55;
                c.g = i&2 ? 0xff : 0x55;
                c.b = i&4 ? 0xff : 0x55;
        } else if (i < 232) {   /* 6x6x6 colour cube. */
                c.r = (i - 16) / 36 * scale / 2;
                c.g = (i - 16) / 6 % 6 * scale / 2;
                c.b = (i - 16) % 6 * scale / 2;
        } else                  /* Grayscale ramp. */
                c.r = c.g = c.b = i * 10 - 2312;
        return c;
}

// iTerm2 sources/NSColor+iTerm.m mashed with vt.c
struct rgb rgb_from_256(int i) {
  struct rgb c; 
  double r, g, b;

  if (i < 8) {            /* Standard colors. */
      c.r = i&1 ? 0xaa : 0x00;
      c.g = i&2 ? 0xaa : 0x00;
      c.b = i&4 ? 0xaa : 0x00;
  } else if (i < 16) {
      c.r = i&1 ? 0xff : 0x55;
      c.g = i&2 ? 0xff : 0x55;
      c.b = i&4 ? 0xff : 0x55;
  } else if (i < 232) {  /* 6x6x6 color cube */
      i = i - 16;
      c.r = (i / 36) ? (i / 36) * 40 + 55 : 0;
      c.g = (i % 36) / 6 ? ((i % 36) / 6) * 40 + 55 : 0;
      c.b = (i % 6) ? (i % 6) * 40 + 55 : 0;
  } else {               /* Greyscale ramp */
      c.r = c.g = c.b = i * 10 - 2312;
  }
  return c;
}

uint8_t rgb_to_idx(struct rgb c) {
  uint8_t incs[6] = { 0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff };
  uint8_t mid[5];

  uint8_t r = 5, g = 5, b = 5;

  // midpoints
  for (int i = 0; i < 5; i++) {
    mid[i] = (incs[i] + incs[i+1]) / 2;
  }

  for (int i = 5; i >= 0; i--) {
    if (c.r < mid[i]) {
      r = i;
    }
    if (c.g < mid[i]) {
      g = i;
    }
    if (c.b < mid[i]) {
      b = i;
    }
  }

  return r * 36 + g * 6 + b + 16;
}


int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("usage: %s [ hexcolor | paletteidx ]\n", argv[0]);
    return 1;
  }

  uint8_t incs[6] = { 0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff };

  uint8_t CLUT[6][6][6]; // color lookup table
  for (int green = 0; green < 6; green++) {
    for (int red = 0; red < 6; red++) {
      for (int blue = 0; blue < 6; blue++) {
        int paletteIdx = 16 + (red * 36) + (green * 6) + blue;
        CLUT[green][red][blue] = paletteIdx;
      }
    }
  }

  // convert 6-char hex string to 24 bit color
  unsigned long number;
  bool p2c = strlen(argv[1]) <= 3; //palette idx -> color

  if (p2c) {
    number = strtol(argv[1], NULL, 10);
  } else {
    number = strtol(argv[1], NULL, 16);
  }

  uint8_t small, big, s1, b1, colorComponent, closest, idx;
  uint8_t newred,newgreen,newblue, ri, gi, bi;

  if (p2c) {
    // Handle palette index to rgb
    struct rgb c = rgb_from_256((int) number); 
    newred   = c.r;
    newgreen = c.g;
    newblue  = c.b;
  } else if(1) {
    struct rgb c = { (number >> 16 ) & 0xFF, (number >> 8 ) & 0xFF, number & 0xFF };
    newred   = c.r;
    newgreen = c.g;
    newblue  = c.b;
    int idx = rgb_to_idx(c);
  } else {
    // rgb to palette index
    uint8_t r, g, b;
    // break up the 24 bits into 8-bit pieces.
    r = (number >> 16 ) & 0xFF;
    g = (number >> 8 ) & 0xFF;
    b = number & 0xFF;

    printf("#%02x%02x%02x = rgb(%d, %d, %d)\n", r, g, b, r, g, b);

    for (int c = 2; c >= 0; c--) {
      colorComponent = (number >> (c * 8)) & 0xFF;
      for (int i = 0; i < 5; i++) {
        small = incs[i];
        big = incs[i+1];
        if (colorComponent >= small && colorComponent <= big) {
          s1 = abs(small - colorComponent);
          b1 = abs(big - colorComponent);
          if (s1 < b1) {
            idx = i;
            closest = small;
          } else {
            idx = i + 1;
            closest = big;
          }

          if (c==2) {
            newred = closest;
            ri = idx;
          } else if (c==1) {
            newgreen = closest;
            gi = idx;
          } else {
            newblue = closest;
            bi = idx;
          }

          break;
        }
      }
    }
  }

  printf("#%02x%02x%02x = rgb(%d, %d, %d): Nearest color in 256 color palette (not considering the grey ramp).\n", newred, newgreen, newblue, newred, newgreen, newblue);
  if (p2c) {
    idx = (int) number;
  } else {
    //idx = CLUT[gi][ri][bi];
    struct rgb c = { (number >> 16 ) & 0xFF, (number >> 8 ) & 0xFF, number & 0xFF };
    idx = rgb_to_idx(c);
    struct rgb cc = rgb_from_256(idx);
    newred = cc.r;
    newgreen = cc.g;
    newblue = cc.b;
  }
  printf("Palette index: %d \033[38;5;%dm(#%02x%02x%02x) \033[48;5;%dm     \033[0m\n", idx, idx, newred, newgreen, newblue, idx );

  return 0;
} 


