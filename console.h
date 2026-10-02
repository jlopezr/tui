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

int  tui_console_init(void);
void tui_console_shutdown(void);

int  tui_console_width(void);
int  tui_console_height(void);

void tui_console_cell(int x, int y, int ch, int attr);
int  tui_console_key(void);
void tui_console_present(void);

#endif
