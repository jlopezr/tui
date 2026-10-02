CC      = cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic
LDLIBS  = -lncurses

TARGET  = demo

all: $(TARGET)

$(TARGET): main.c tui.c tui.h console.h console_ncurses.c demo.c
	$(CC) $(CFLAGS) -DTUI_BACKEND_NCURSES main.c -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean