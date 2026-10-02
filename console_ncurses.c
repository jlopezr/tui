#include <ncurses.h>
#include "console.h"

static int tui_colors;

int tui_console_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    tui_colors = 0;

    if (has_colors()) {
        start_color();
        tui_colors = 1;

        /*
         * attr encodes DOS-like fg/bg nibbles.
         * We create pairs lazily in tui_console_cell().
         * ncurses pair 1..255 are enough for this demo.
         */
    }

    curs_set(0);
    erase();
    return 1;
}

void tui_console_shutdown(void)
{
    endwin();
}

int tui_console_width(void)
{
    int h;
    int w;

    getmaxyx(stdscr, h, w);
    (void)h;
    return w;
}

int tui_console_height(void)
{
    int h;
    int w;

    getmaxyx(stdscr, h, w);
    (void)w;
    return h;
}

static short tui_nc_color(int c)
{
    switch (c & 7) {
    case 0: return COLOR_BLACK;
    case 1: return COLOR_BLUE;
    case 2: return COLOR_GREEN;
    case 3: return COLOR_CYAN;
    case 4: return COLOR_RED;
    case 5: return COLOR_MAGENTA;
    case 6: return COLOR_YELLOW;
    default: return COLOR_WHITE;
    }
}

void tui_console_cell(int x, int y, int ch, int attr)
{
    int fg;
    int bg;
    int pair;
    chtype a;

    a = 0;

    if (tui_colors) {
        fg = attr & 0x0f;
        bg = (attr >> 4) & 0x0f;
        pair = 1 + ((bg & 7) << 3) + (fg & 7);

        init_pair((short)pair,
                  tui_nc_color(fg),
                  tui_nc_color(bg));

        a |= COLOR_PAIR(pair);

        if (fg & 8)
            a |= A_BOLD;
    }

    mvaddch(y, x, ((unsigned char)ch) | a);
}

int tui_console_key(void)
{
    int ch;

    ch = getch();

    switch (ch) {
    case '\n':
    case '\r':
        return TUI_KEY_ENTER;

    case 27:
        return TUI_KEY_ESCAPE;

    case '\t':
        return TUI_KEY_TAB;

#ifdef KEY_BTAB
    case KEY_BTAB:
        return TUI_KEY_BACKTAB;
#endif

    case KEY_UP:
        return TUI_KEY_UP;

    case KEY_DOWN:
        return TUI_KEY_DOWN;

    case KEY_LEFT:
        return TUI_KEY_LEFT;

    case KEY_RIGHT:
        return TUI_KEY_RIGHT;

    case KEY_F(1):  return TUI_KEY_F1;
    case KEY_F(2):  return TUI_KEY_F2;
    case KEY_F(3):  return TUI_KEY_F3;
    case KEY_F(4):  return TUI_KEY_F4;
    case KEY_F(5):  return TUI_KEY_F5;
    case KEY_F(6):  return TUI_KEY_F6;
    case KEY_F(7):  return TUI_KEY_F7;
    case KEY_F(8):  return TUI_KEY_F8;
    case KEY_F(9):  return TUI_KEY_F9;
    case KEY_F(10): return TUI_KEY_F10;
    case KEY_F(11): return TUI_KEY_F11;
    case KEY_F(12): return TUI_KEY_F12;


    default:
        return ch;
    }
}

void tui_console_present(void)
{
    refresh();
}
