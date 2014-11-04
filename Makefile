# Compile bonepeg.
# Requires:  g++, pkg-config, opencv, ncurses
CC = g++
CFLAGS = -O2
LDFLAGS =

#CFLAGS  += -Wall
CFLAGS  += $(shell pkg-config --cflags opencv)
CFLAGS  += $(shell pkg-config --cflags ncurses)
LDFLAGS += $(shell pkg-config --libs opencv)
LDFLAGS += $(shell pkg-config --libs ncurses)

all: bonepeg

bonepeg: bonepeg.cpp
	$(CC) $(CFLAGS) bonepeg.cpp $(LDFLAGS) -o bonepeg

clean:
	@rm -rf *.o bonepeg
