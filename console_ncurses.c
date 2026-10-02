#include <stdio.h>
#include <ncurses.h>
#include "console.h"

static int tui_colors;

static int tui_mouse_x;
static int tui_mouse_y;
static int tui_mouse_action;
static int tui_mouse_buttons;

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

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(200);

    /*
     * Ask xterm-like terminals for any-motion reporting; some
     * ncurses versions only enable button events themselves.
     */
    putp("\033[?1003h");
    fflush(stdout);

    curs_set(0);
    erase();
    return 1;
}

void tui_console_shutdown(void)
{
    putp("\033[?1003l");
    fflush(stdout);
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
    chtype c;

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

    switch (ch) {
    case TUI_CH_HLINE:
        c = ACS_HLINE;
        break;

    case TUI_CH_VLINE:
        c = ACS_VLINE;
        break;

    case TUI_CH_TL:
        c = ACS_ULCORNER;
        break;

    case TUI_CH_TR:
        c = ACS_URCORNER;
        break;

    case TUI_CH_BL:
        c = ACS_LLCORNER;
        break;

    case TUI_CH_BR:
        c = ACS_LRCORNER;
        break;

    case TUI_CH_LTEE:
        c = ACS_LTEE;
        break;

    case TUI_CH_RTEE:
        c = ACS_RTEE;
        break;

    case TUI_CH_TTEE:
        c = ACS_TTEE;
        break;

    case TUI_CH_BTEE:
        c = ACS_BTEE;
        break;

    case TUI_CH_CROSS:
        c = ACS_PLUS;
        break;

    default:
        c = (chtype)(unsigned char)ch;
        break;
    }

    mvaddch(y, x, c | a);
}

static int tui_translate_mouse(void)
{
    MEVENT ev;
    int changed;

    if (getmouse(&ev) != OK)
        return 0;

    tui_mouse_x = ev.x;
    tui_mouse_y = ev.y;
    tui_mouse_action = TUI_MOUSE_MOVE;
    tui_mouse_buttons = 0;
    changed = 0;

#ifdef BUTTON1_DOUBLE_CLICKED
    if (ev.bstate & BUTTON1_DOUBLE_CLICKED) {
        tui_mouse_action = TUI_MOUSE_DOUBLE;
        changed = TUI_MOUSE_LEFT;
    } else
#endif
    if (ev.bstate & BUTTON1_PRESSED) {
        tui_mouse_action = TUI_MOUSE_DOWN;
        changed = TUI_MOUSE_LEFT;
    } else if (ev.bstate & BUTTON1_RELEASED) {
        tui_mouse_action = TUI_MOUSE_UP;
        changed = TUI_MOUSE_LEFT;
    } else if (ev.bstate & BUTTON3_PRESSED) {
        tui_mouse_action = TUI_MOUSE_DOWN;
        changed = TUI_MOUSE_RIGHT;
    } else if (ev.bstate & BUTTON3_RELEASED) {
        tui_mouse_action = TUI_MOUSE_UP;
        changed = TUI_MOUSE_RIGHT;
    } else if (ev.bstate & BUTTON2_PRESSED) {
        tui_mouse_action = TUI_MOUSE_DOWN;
        changed = TUI_MOUSE_MIDDLE;
    } else if (ev.bstate & BUTTON2_RELEASED) {
        tui_mouse_action = TUI_MOUSE_UP;
        changed = TUI_MOUSE_MIDDLE;
    }

    tui_mouse_buttons = changed;

    return 1;
}

void tui_console_mouse(int *x, int *y, int *action, int *buttons)
{
    *x = tui_mouse_x;
    *y = tui_mouse_y;
    *action = tui_mouse_action;
    *buttons = tui_mouse_buttons;
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

    case KEY_MOUSE:
        return tui_translate_mouse() ? TUI_KEY_MOUSE
                                     : TUI_KEY_NONE;

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
    case KEY_HOME:
        return TUI_KEY_HOME;

    case KEY_END:
        return TUI_KEY_END;

    case KEY_PPAGE:
        return TUI_KEY_PAGEUP;

    case KEY_NPAGE:
        return TUI_KEY_PAGEDOWN;

    case KEY_DC:
        return TUI_KEY_DELETE;

    case KEY_IC:
        return TUI_KEY_INSERT;

    case KEY_BACKSPACE:
    case 127:
    case 8:

        return TUI_KEY_BACKSPACE;

    default:
        return ch;
    }
}

void tui_console_cursor(int x, int y, int visible)
{
    if (visible) {
        curs_set(1);
        move(y, x);
    } else {
        curs_set(0);
    }
}

void tui_console_present(void)
{
    refresh();
}
