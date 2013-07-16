CC=gcc
CFLAGS=
LIBS=-lncurses -ltinfo
DEPS=
PROGS=ncurses_test nccolor greyramp hex2pal ttycolors shift

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

all: $(PROGS)

ncurses_test: ncurses_test.c
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

nccolor: nccolor.c
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

greyramp: greyramp.c
	$(CC) -o $@ $^ $(LIBS) -std=c99 $(CFLAGS)

shift: shift.c
	$(CC) -o $@ $^ $(LIBS) -std=c99 $(CFLAGS)

hex2pal: hex2pal.c
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

ttycolors: ttycolors.c
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

clean:
#	rm -f *.o *~ core
	rm -f *.o *~ core $(PROGS)
