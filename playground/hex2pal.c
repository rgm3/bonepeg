// Changes some entries in the palette, starting at index color_index
/* e.g. "TERM=xterm-256color ./test_colors 3366FF FF0000 3a4682" */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
	int i;

	initscr();
	start_color();

	if (!can_change_color()) {
		printw("can't change colors :(\n");
		refresh();
		sleep(2);
		endwin();
		exit(0);
	};

	printw("can change colors.\n");
	printw("%d colors, %d pairs available\n", COLORS, COLOR_PAIRS);

	int color_index = 3;
	int pair_index = 2;
	double magic = 1000/255.0;

	/* init_color( */
	for(i = 1; i < argc; ++i) {
		const char* hexcolor = argv[i];
		int r, g, b;
		if (hexcolor[0] == '#') {
			++hexcolor;
		}

		sscanf(hexcolor, "%02x%02x%02x", &r, &g, &b);
		init_color(color_index,
			(int)(r * magic),
			(int)(g * magic),
			(int)(b * magic));
		init_pair(pair_index, color_index, 0);
		attrset(COLOR_PAIR(pair_index));
		printw("#%02x%02x%02x\n", r, g, b);
		refresh();
		++pair_index;
		++color_index;
	}

	getch();


	endwin();
}
