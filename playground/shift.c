/* Print out color pairs
 * gcc -std=c99 -lncurses shift.c
 * Experiment in palette redefinition and restoration.
 * Really screws up your color scheme good.
 */

#include <stdlib.h>
#include <curses.h>

/* After redefining the palette, "restore" it by writing sane defaults
   Proper palette restoration to exact previous settings not working via ncurses endwin()
   And I can't properly read the existing settings, at least not from Gnome Terminal
   See: http://stackoverflow.com/questions/8686368/how-can-the-original-terminal-palette-be-acquired-preferably-using-ncurses-rout
   ported (with basic color additions) from 256colors.pl by Todd Larason <jtl@molehill.org>
*/
void restorePalette() {
    int red, green, blue, idx, grey, level;
    const float scale = 1000 / 6;
     
    // colors 16-231 are a 6x6x6 color cube
    for (red = 0; red < 6; red++) { 
        for (green = 0; green < 6; green++) { 
            for (blue = 0; blue < 6; blue++) { 
                idx = 16 + (red * 36) + (green * 6) + blue;
                init_color(idx, (int)(red * scale), (int)(green * scale), (int)(blue * scale));
            } 
        }
    } 

    // colors 232-255 are a grayscale ramp, intentionally leaving out black and white
    for (grey = 0; grey < 24; grey++) {
        level = (int)(grey * 1000/26);
        init_color( 232 + grey, level, level, level);
    }

    // basic colors, from xterm_color_chart.py by Ian Ward
    // values copied from X11/rgb.txt and XTerm-col.ad:
    // Uses a byte range (0-255) but init_color wants 0-1000.
    int basics[][3] = {
        {0,0,0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0},
        {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
        {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
        {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}
    };
    for (idx = 0; idx < 16; idx++) {
        red = (int) basics[idx][0] * 1000 / 255;
        green = (int) basics[idx][1] * 1000 / 255;
        blue = (int) basics[idx][2] * 1000 / 255;
        init_color(idx, red, green, blue);
    }
}

void setGreys() {
  const int maxColors = 256;

  for(int i = 0; i < maxColors; i++) {
      int greyLevel =  (((i + 1) * 1000) / maxColors);
      init_color(i, greyLevel, greyLevel, greyLevel);
  }

}

void dumpPalette() {
  int pidx = 1;
  const int maxColors = 256;
  for(int i = 0; i < maxColors; i++) {
      init_pair(pidx, maxColors - i, i);
      attron(COLOR_PAIR(pidx));
      if (i % 16 == 0) {
          printw("\n");
      }
      addch('@');
      addch(' ');
      attroff(COLOR_PAIR(pidx));

      //mvprintw(0, i, "%s", "#");
      pidx++;
  }
  printw("\n");
}

int main(int argc, char **argv) {
  if(!initscr()) {
    printf("Error initializing screen.\n");
    exit(1);
  }
  if(!has_colors()) {
    endwin();
    printf("This terminal does not support colours.\n");
    exit(1);
  }
  if(can_change_color() == FALSE) {
    endwin();
    printf("This terminal does not support redefining colours.\n");
    exit(1);
  }
  noecho();
  start_color();

  // this prevents ncurses from forcing a white-on-black colour scheme,
  // regardless of what scheme is used by the terminal.
  //use_default_colors();
  //
  printw("256 grays:\n");
  refresh();

  setGreys();
  dumpPalette();
  refresh();
  getch();

  restorePalette();
  printw("Palette after restore:\n");
  dumpPalette();
  refresh();
  getch();

  curs_set(1);
  endwin();
  return 0;
}

