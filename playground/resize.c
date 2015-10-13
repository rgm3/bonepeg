#include <ncurses.h>

int main(void) {
  WINDOW * stdscr = initscr();
  int width, height;
  while (1) {
    getmaxyx(stdscr, height, width);
    mvprintw(0,0,"%d %d %o\n", height, width, getch());
    refresh();
  }
  return 0;
}
