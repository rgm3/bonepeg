all: bonepeg

bonepeg: bonepeg.cpp
	g++ -O2 `pkg-config --cflags --libs opencv` bonepeg.cpp -o bonepeg
