CC      = cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic
LDLIBS  = -lncurses

OBJS = tui.o console_ncurses.o demo.o

all: demo

demo: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

tui.o: tui.c tui.h console.h
	$(CC) $(CFLAGS) -c tui.c

console_ncurses.o: console_ncurses.c console.h
	$(CC) $(CFLAGS) -c console_ncurses.c

demo.o: demo.c tui.h console.h
	$(CC) $(CFLAGS) -c demo.c

clean:
	rm -f $(OBJS) demo

.PHONY: all clean
