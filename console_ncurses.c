#include <stdio.h>
#include <sys/time.h>
#include <ncurses.h>
#include "console.h"

#define TUI_MOUSE_DOUBLE_CLICK_MS 200

static int tui_colors;

static int tui_mouse_x;
static int tui_mouse_y;
static int tui_mouse_action;
static int tui_mouse_buttons;
static int tui_mouse_left_pressed;
static int tui_mouse_left_moved;
static int tui_mouse_press_x;
static int tui_mouse_press_y;
static int tui_mouse_press_time_valid;
static struct timeval tui_mouse_press_time;
static int tui_mouse_last_click_valid;
static int tui_mouse_last_click_x;
static int tui_mouse_last_click_y;
static struct timeval tui_mouse_last_click_time;

static long tui_mouse_elapsed_ms(struct timeval *later,
                                 struct timeval *earlier)
{
    return (long)(later->tv_sec - earlier->tv_sec) * 1000L +
           (long)(later->tv_usec - earlier->tv_usec) / 1000L;
}

static int tui_mouse_is_double_click(int x, int y)
{
    struct timeval now;
    long elapsed_ms;

    if (!tui_mouse_last_click_valid ||
        x != tui_mouse_last_click_x ||
        y != tui_mouse_last_click_y)
        return 0;

    if (gettimeofday(&now, 0) != 0) {
        tui_mouse_last_click_valid = 0;
        return 0;
    }

    elapsed_ms = tui_mouse_elapsed_ms(
        &now, &tui_mouse_last_click_time);

    tui_mouse_last_click_valid = 0;

    return elapsed_ms >= 0 &&
           elapsed_ms <= TUI_MOUSE_DOUBLE_CLICK_MS;
}

int tui_console_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    tui_mouse_left_pressed = 0;
    tui_mouse_left_moved = 0;
    tui_mouse_press_time_valid = 0;
    tui_mouse_last_click_valid = 0;

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

    mousemask(BUTTON1_PRESSED |
              BUTTON1_RELEASED |
              BUTTON2_PRESSED |
              BUTTON2_RELEASED |
              BUTTON3_PRESSED |
              BUTTON3_RELEASED |
              REPORT_MOUSE_POSITION,
              NULL);
    mouseinterval(0);

    /*
     * Track motion while a button is held without delaying the
     * press event to resolve clicks.
     */
    putp("\033[?1002h");
    fflush(stdout);

    curs_set(0);
    erase();
    return 1;
}

void tui_console_shutdown(void)
{
    tui_mouse_left_pressed = 0;
    tui_mouse_press_time_valid = 0;
    tui_mouse_last_click_valid = 0;
    putp("\033[?1002l");
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
    long elapsed_ms;
    struct timeval now;

    if (getmouse(&ev) != OK)
        return 0;

    tui_mouse_x = ev.x;
    tui_mouse_y = ev.y;
    tui_mouse_action = TUI_MOUSE_MOVE;
    tui_mouse_buttons = 0;
    changed = 0;

    if (ev.bstate & BUTTON1_PRESSED) {
        tui_mouse_action =
            tui_mouse_is_double_click(ev.x, ev.y)
                ? TUI_MOUSE_DOUBLE
                : TUI_MOUSE_DOWN;
        changed = TUI_MOUSE_LEFT;
        tui_mouse_left_pressed = 1;
        tui_mouse_left_moved = 0;
        tui_mouse_press_x = ev.x;
        tui_mouse_press_y = ev.y;
        tui_mouse_press_time_valid =
            gettimeofday(&tui_mouse_press_time, 0) == 0;
    } else if (ev.bstate & BUTTON1_RELEASED) {
        tui_mouse_action = TUI_MOUSE_UP;
        changed = TUI_MOUSE_LEFT;
        if (tui_mouse_left_pressed &&
            !tui_mouse_left_moved &&
            ev.x == tui_mouse_press_x &&
            ev.y == tui_mouse_press_y &&
            tui_mouse_press_time_valid &&
            gettimeofday(&now, 0) == 0) {
            elapsed_ms =
                tui_mouse_elapsed_ms(&now, &tui_mouse_press_time);

            if (elapsed_ms >= 0 &&
                elapsed_ms <= TUI_MOUSE_DOUBLE_CLICK_MS) {
                tui_mouse_last_click_time = now;
                tui_mouse_last_click_valid = 1;
                tui_mouse_last_click_x = ev.x;
                tui_mouse_last_click_y = ev.y;
            } else {
                tui_mouse_last_click_valid = 0;
            }
        } else {
            tui_mouse_last_click_valid = 0;
        }
        tui_mouse_left_pressed = 0;
        tui_mouse_press_time_valid = 0;
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
#ifdef REPORT_MOUSE_POSITION
    } else if (ev.bstate & REPORT_MOUSE_POSITION) {
        if (tui_mouse_left_pressed &&
            (ev.x != tui_mouse_press_x ||
             ev.y != tui_mouse_press_y))
            tui_mouse_left_moved = 1;
#endif
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
