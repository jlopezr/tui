#ifndef TUI_CONSOLE_H
#define TUI_CONSOLE_H

#define TUI_KEY_NONE       0
#define TUI_KEY_ENTER      0x100
#define TUI_KEY_ESCAPE     0x101
#define TUI_KEY_TAB        0x102
#define TUI_KEY_BACKTAB    0x103
#define TUI_KEY_UP         0x110
#define TUI_KEY_DOWN       0x111
#define TUI_KEY_LEFT       0x112
#define TUI_KEY_RIGHT      0x113
#define TUI_KEY_HOME       0x114
#define TUI_KEY_END        0x115
#define TUI_KEY_DELETE     0x116
#define TUI_KEY_INSERT     0x117
#define TUI_KEY_BACKSPACE  0x118
#define TUI_KEY_PAGEUP     0x119
#define TUI_KEY_PAGEDOWN   0x11a

/* Returned by tui_console_key(); details via tui_console_mouse(). */
#define TUI_KEY_MOUSE      0x130

/* Mouse actions and buttons. */
#define TUI_MOUSE_MOVE     1
#define TUI_MOUSE_DOWN     2
#define TUI_MOUSE_UP       3
#define TUI_MOUSE_DOUBLE  4

#define TUI_MOUSE_LEFT     0x01
#define TUI_MOUSE_RIGHT    0x02
#define TUI_MOUSE_MIDDLE   0x04

#define TUI_KEY_F1         0x120
#define TUI_KEY_F2         0x121
#define TUI_KEY_F3         0x122
#define TUI_KEY_F4         0x123
#define TUI_KEY_F5         0x124
#define TUI_KEY_F6         0x125
#define TUI_KEY_F7         0x126
#define TUI_KEY_F8         0x127
#define TUI_KEY_F9         0x128
#define TUI_KEY_F10        0x129
#define TUI_KEY_F11        0x12a
#define TUI_KEY_F12        0x12b

/*
 * ------------------------------------------------------------
 * Graphic characters
 *
 * Values >= 0x100 are abstract console characters.
 * Each backend translates them to its native representation.
 * ------------------------------------------------------------
 */

#define TUI_CH_HLINE   0x100
#define TUI_CH_VLINE   0x101

#define TUI_CH_TL      0x102
#define TUI_CH_TR      0x103
#define TUI_CH_BL      0x104
#define TUI_CH_BR      0x105

#define TUI_CH_LTEE    0x106
#define TUI_CH_RTEE    0x107
#define TUI_CH_TTEE    0x108
#define TUI_CH_BTEE    0x109

#define TUI_CH_CROSS   0x10A

/* Double-line frame; backends may degrade them to single lines. */
#define TUI_CH_DHLINE  0x10B
#define TUI_CH_DVLINE  0x10C
#define TUI_CH_DTL     0x10D
#define TUI_CH_DTR     0x10E
#define TUI_CH_DBL     0x10F
#define TUI_CH_DBR     0x110

/* Scroll bar parts. */
#define TUI_CH_UP_TRIANGLE     0x111
#define TUI_CH_DOWN_TRIANGLE   0x112
#define TUI_CH_LEFT_TRIANGLE   0x113
#define TUI_CH_RIGHT_TRIANGLE  0x114
#define TUI_CH_SCROLL_TRACK    0x115
#define TUI_CH_SCROLL_THUMB    0x116

/* Check box mark and radio button dot. */
#define TUI_CH_CHECK           0x117
#define TUI_CH_BULLET          0x118

/* ASCII imprimible: lo que aceptan los controles de texto por defecto. */
#define TUI_ASCII_PRINTABLE(key) ((key) >= 32 && (key) <= 126)

int  tui_console_init(void);
void tui_console_shutdown(void);

/*
 * Whether a key code returned by tui_console_key() is a character that text
 * controls may insert. It is a property of the console, not of the library:
 * the PC backends return bytes or UTF-8 pieces above 127 that the cell writer
 * cannot draw, so they accept only ASCII; the MiniCPU console draws CP437 and
 * accepts 128..255 as well (n with tilde, accented vowels...).
 */
int  tui_console_printable(int key);

/*
 * Whether a mouse can deliver events right now. Keys are always available (a
 * terminal, the serial line, a keyboard), so there is no such question for them;
 * the mouse is optional and on some machines it comes and goes while the program
 * runs (the MiniCPU console only has one when something is plugged into its INPUT
 * block), so ask when it matters rather than once at start-up. An application can
 * use it to show or hide hints about clicking, or to skip drawing a pointer.
 */
int  tui_console_has_mouse(void);

int  tui_console_width(void);
int  tui_console_height(void);

void tui_console_cell(int x, int y, int ch, int attr);

/*
 * Input, in two forms over the same events:
 *
 *   tui_console_poll()  the next key (or TUI_KEY_MOUSE), or TUI_KEY_NONE if there
 *                       is nothing right now. It never waits. What the TUI has no
 *                       use for (a key release, a modifier on its own, a mouse
 *                       move inside the same cell) is skipped inside, so NONE
 *                       really means an empty queue and not "something ignored".
 *   tui_console_key()   waits for the next one. It returns TUI_KEY_NONE only if the
 *                       input is gone (closed, or an error).
 *
 * The mouse details of a TUI_KEY_MOUSE are only good until the next call to either
 * (tui_console_mouse() reads them), so take them right away.
 */
int  tui_console_poll(void);
int  tui_console_key(void);

/*
 * Fetches the mouse event announced by TUI_KEY_MOUSE.
 * Coordinates are screen cells. buttons is the button that
 * changed in a DOWN/UP event, and 0 for MOVE.
 */
void tui_console_mouse(int *x, int *y, int *action, int *buttons);
void tui_console_cursor(int x, int y, int visible);
void tui_console_present(void);

#endif
