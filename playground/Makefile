CC = gcc
CPP = g++
CFLAGS= -O2
#LDFLAGS=-lncurses -ltinfo
LDFLAGS = -lncurses
CV_LDFLAGS = $(shell pkg-config --libs opencv)
CV_CFLAGS = $(shell pkg-config --cflags opencv)
DEPS =
PROGS = ncurses_test nccolor greyramp hex2pal ttycolors shift colortrans sliders

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

all: $(PROGS)

ncurses_test: ncurses_test.c
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

nccolor: nccolor.c
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

greyramp: greyramp.c
	$(CC) -o $@ $^ $(LDFLAGS) -std=c99 $(CFLAGS)

shift: shift.c
	$(CC) -o $@ $^ $(LDFLAGS) -std=c99 $(CFLAGS)

hex2pal: hex2pal.c
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

ttycolors: ttycolors.c
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

colortrans: colortrans.c
	$(CC) -o $@ $^ $(CFLAGS)

sliders: sliders.cpp
	$(CPP) $(CV_CFLAGS) $(CV_LDFLAGS) -o $@  $^ 

clean:
#	rm -f *.o *~ core
	rm -f *.o *~ core $(PROGS)
