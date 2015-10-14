# Compile bonepeg.
# Requires:  g++, pkg-config, opencv, ncurses
CC = g++
CFLAGS = -O2 -Wall -Wno-unused-local-typedefs
LDFLAGS =

#CFLAGS  += -Wall
CFLAGS  += $(shell pkg-config --cflags opencv)
CFLAGS  += $(shell pkg-config --cflags ncurses)
LDFLAGS += $(shell pkg-config --libs opencv)
LDFLAGS += $(shell pkg-config --libs ncurses)
DEPS = colorhelpers.hpp
OBJ = colorhelpers.o bonepeg.o

all: bonepeg

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

bonepeg: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	@rm -rf *.o bonepeg
