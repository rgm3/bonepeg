/* Print out color pairs
 * gcc -std=c99 -lncurses -ltinfo pairs.c
 */

#include <stdlib.h>
#include <curses.h>

int main(int argc, char **argv) {
  if(!initscr()) {
    printf("Error initializing screen.\n");
    exit(1);
  }
  if(!has_colors()) {
    printf("This terminal does not support colours.\n");
    exit(1);
  }
  if(!can_change_color()) {
    printf("This terminal does not support redefining colours.\n");
    exit(1);
  }
  start_color();

  // this prevents ncurses from forcing a white-on-black colour scheme,
  // regardless of what scheme is used by the terminal.
  //use_default_colors();

  int pidx = 1; // start pair indexes at 2

  int maxColors = 256;

  for(int i = 16; i < maxColors; i++) {
      int greyLevel =  (((i + 1) * 1000) / maxColors);
      init_color(i, greyLevel, greyLevel, greyLevel);
      init_pair( pidx, 0, i);
      
      attron(COLOR_PAIR(pidx));
      addch(' ');
      addch(' ');
      if (i % 15 == 0) {
          //printw("\n");
      }

      pidx++;
      //mvprintw(0, i, "%s", "#");
  }

  refresh();
  getch();
  endwin();
  return 0;
}

