// From https://gist.github.com/InfernoZeus/5536379

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
  start_color();
 
  // this prevents ncurses from forcing a white-on-black colour scheme,
  // regardless of what scheme is used by the terminal.
  use_default_colors();
 
  curs_set(0);
 
  mvprintw(0, 0, "Number of colors: %d", COLORS);
  mvprintw(1, 0, "Number of pairs: %d", COLOR_PAIRS);
 
  short r, g, b, COLOR_TEST=9; 
  color_content(COLOR_WHITE,&r,&g,&b);
  mvprintw(3, 0, "COLOR_WHITE: (%d, %d, %d)", r, g, b);
  init_color(COLOR_TEST, r+1, g+1, b+1);
  color_content(COLOR_TEST,&r,&g,&b);
  mvprintw(4, 0, "COLOR_TEST: (%d, %d, %d)", r, g, b);
 
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_TEST, COLOR_BLACK);
 
  attron(COLOR_PAIR(1));
  mvaddstr(6, 0, "In COLOR_WHITE.");
  attron(A_BOLD);
  mvaddstr(6, 20, "In BOLD COLOR_WHITE.");
  attroff(COLOR_PAIR(1) | A_BOLD);
 
  attron(COLOR_PAIR(2));
  mvaddstr(7, 0, "In COLOR_TEST.");
  attron(A_BOLD);
  mvaddstr(7, 20, "In BOLD COLOR_TEST.");
  attroff(COLOR_PAIR(2) | A_BOLD);
 
  refresh();
  getch();
  endwin();
  return 0;
}
