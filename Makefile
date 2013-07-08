all: bonepeg

bonepeg: bonepeg.cpp
	g++ -O2 `pkg-config --cflags --libs opencv` `pkg-config --cflags --libs ncurses` bonepeg.cpp -o bonepeg
