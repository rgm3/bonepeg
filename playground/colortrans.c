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

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("usage: %s hexcolor\n", argv[0]);
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
  unsigned long number = strtol(argv[1], NULL, 16);
  uint8_t r, g, b;

  // break up the 24 bits into 8-bit pieces.
  r = (number >> 16 ) & 0xFF;
  g = (number >> 8 ) & 0xFF;
  b = number & 0xFF;

  printf("#%02x%02x%02x = rgb(%d, %d, %d)\n", r, g, b, r, g, b);

  uint8_t small, big, s1, b1, colorComponent, closest, idx;

  uint8_t newred,newgreen,newblue, ri, gi, bi;
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

  printf("#%02x%02x%02x = rgb(%d, %d, %d): Nearest color in 256 color palette.\n", newred, newgreen, newblue, newred, newgreen, newblue);
  idx = CLUT[gi][ri][bi];
  printf("Palette index: %d \033[38;5;%dm(#%02x%02x%02x)\033[0m\n", idx, idx, newred, newgreen, newblue );

  return 0;
} 


