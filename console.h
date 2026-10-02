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

/* Returned by tui_console_key(); details via tui_console_mouse(). */
#define TUI_KEY_MOUSE      0x130

/* Mouse actions and buttons. */
#define TUI_MOUSE_MOVE     1
#define TUI_MOUSE_DOWN     2
#define TUI_MOUSE_UP       3

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

int  tui_console_init(void);
void tui_console_shutdown(void);

int  tui_console_width(void);
int  tui_console_height(void);

void tui_console_cell(int x, int y, int ch, int attr);
int  tui_console_key(void);

/*
 * Fetches the mouse event announced by TUI_KEY_MOUSE.
 * Coordinates are screen cells. For DOWN/UP, buttons is the
 * button that changed; for MOVE, the buttons currently held.
 */
void tui_console_mouse(int *x, int *y, int *action, int *buttons);
void tui_console_cursor(int x, int y, int visible);
void tui_console_present(void);

#endif
