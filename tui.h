#ifndef TUI_H
#define TUI_H

#include "console.h"

typedef struct TuiControl TuiControl;
typedef struct TuiClass TuiClass;
typedef struct TuiDraw TuiDraw;
typedef struct TuiEvent TuiEvent;

#define TUI_VISIBLE    0x0001
#define TUI_ENABLED    0x0002
#define TUI_FOCUSABLE  0x0004
#define TUI_TABSTOP    0x0008

#define TUI_EV_KEY      1
#define TUI_EV_COMMAND  2

#define TUI_CMD_NONE    0

#define TUI_BLACK       0
#define TUI_BLUE        1
#define TUI_GREEN       2
#define TUI_CYAN        3
#define TUI_RED         4
#define TUI_MAGENTA     5
#define TUI_BROWN       6
#define TUI_LIGHTGRAY   7
#define TUI_DARKGRAY    8
#define TUI_LIGHTBLUE   9
#define TUI_LIGHTGREEN  10
#define TUI_LIGHTCYAN   11
#define TUI_LIGHTRED    12
#define TUI_LIGHTMAGENTA 13
#define TUI_YELLOW      14
#define TUI_WHITE       15

#define TUI_ATTR(fg,bg) ((fg) | ((bg) << 4))

struct TuiDraw {
    int ox;
    int oy;
    int x1;
    int y1;
    int x2;
    int y2;
};

struct TuiEvent {
    int type;
    int key;
    int command;
    TuiControl *source;
};

struct TuiClass {
    void (*draw)(TuiControl *control, TuiDraw *draw);
    int  (*event)(TuiControl *control, TuiEvent *event);
};

struct TuiControl {
    const TuiClass *cls;

    TuiControl *parent;
    TuiControl *first;
    TuiControl *last;
    TuiControl *next;
    TuiControl *prev;

    int x;
    int y;
    int width;
    int height;
    int flags;
};

typedef struct TuiWindow {
    TuiControl control;
    const char *title;
    TuiControl *focused;
} TuiWindow;

typedef struct TuiLabel {
    TuiControl control;
    const char *text;
} TuiLabel;

typedef struct TuiButton {
    TuiControl control;
    const char *text;
    int command;
} TuiButton;

int  tui_init(void);
void tui_shutdown(void);

void tui_add(TuiControl *parent, TuiControl *child);
void tui_draw(TuiControl *root);
int  tui_dispatch(TuiWindow *window, TuiEvent *event);
int  tui_read_event(TuiEvent *event);

void tui_window_init(TuiWindow *window,
                     int x, int y, int width, int height,
                     const char *title);

void tui_label_init(TuiLabel *label,
                    int x, int y,
                    const char *text);

void tui_button_init(TuiButton *button,
                     int x, int y, int width,
                     const char *text,
                     int command);

void tui_label_set_text(TuiLabel *label, const char *text);

#endif
