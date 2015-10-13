#!/usr/bin/env python

import curses

def main(stdscr):
    curses.start_color()
    curses.use_default_colors()

    for i in range(0, curses.COLORS):
        curses.init_pair(i, -1, i)

    try:
        for i in range(0, 255):
#            stdscr.addstr(str(i), curses.color_pair(i))
            strhex = "%0.2X " % i;
            stdscr.addstr(strhex, curses.color_pair(i))
            if i > 4 and (i - 9) % 6 == 0:
                stdscr.addstr("\n");
    except curses.ERR:
        # End of screen reached
        pass

    stdscr.getch()

curses.wrapper(main)
