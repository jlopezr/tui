#ifndef TUI_H
#define TUI_H

#include "console.h"

/*
 * Forward declarations
 */
typedef struct TuiControl TuiControl;
typedef struct TuiClass TuiClass;
typedef struct TuiDraw TuiDraw;
typedef struct TuiEvent TuiEvent;

typedef struct TuiDesktop TuiDesktop;
typedef struct TuiWindow TuiWindow;
typedef struct TuiLabel TuiLabel;
typedef struct TuiButton TuiButton;

/*
 * Control flags
 */
#define TUI_VISIBLE       0x0001
#define TUI_ENABLED       0x0002
#define TUI_FOCUSABLE     0x0004
#define TUI_TABSTOP       0x0008
#define TUI_GLOBAL        0x0010


/*
 * Event types
 */
#define TUI_EV_KEY      1
#define TUI_EV_COMMAND  2


/*
 * Commands
 */
#define TUI_CMD_NONE    0


/*
 * Colors
 *
 * Deliberately DOS-like. The console backend is responsible
 * for converting these values to the native representation.
 */
#define TUI_BLACK          0
#define TUI_BLUE           1
#define TUI_GREEN          2
#define TUI_CYAN           3
#define TUI_RED            4
#define TUI_MAGENTA        5
#define TUI_BROWN          6
#define TUI_LIGHTGRAY      7
#define TUI_DARKGRAY       8
#define TUI_LIGHTBLUE      9
#define TUI_LIGHTGREEN    10
#define TUI_LIGHTCYAN     11
#define TUI_LIGHTRED      12
#define TUI_LIGHTMAGENTA  13
#define TUI_YELLOW        14
#define TUI_WHITE         15

#define TUI_ATTR(fg,bg) ((fg) | ((bg) << 4))

/*
 * Predefined themes i.e. attribute values.
 */
#define TUI_ATTR_DESKTOP        TUI_ATTR(TUI_LIGHTGRAY, TUI_CYAN)
#define TUI_ATTR_WINDOW         TUI_ATTR(TUI_WHITE,     TUI_BLUE)
#define TUI_ATTR_MENUBAR        TUI_ATTR(TUI_BLACK,     TUI_LIGHTGRAY)
#define TUI_ATTR_MENU_SELECTED  TUI_ATTR(TUI_WHITE,     TUI_BLUE)
#define TUI_ATTR_DISABLED       TUI_ATTR(TUI_DARKGRAY,  TUI_LIGHTGRAY)
#define TUI_ATTR_STATUSBAR      TUI_ATTR(TUI_BLACK,     TUI_LIGHTGRAY)
#define TUI_ATTR_LABEL          TUI_ATTR(TUI_LIGHTGRAY, TUI_BLUE)

/*
 * Drawing context.
 *
 * ox/oy:
 *     Origin of the current control in screen coordinates.
 *
 * x1/y1/x2/y2:
 *     Current clipping rectangle in screen coordinates.
 *     x2/y2 are exclusive.
 */
struct TuiDraw {
    int ox;
    int oy;

    int x1;
    int y1;
    int x2;
    int y2;
};


/*
 * Event
 */
struct TuiEvent {
    int type;

    int key;
    int command;

    TuiControl *source;
};


/*
 * Class / vtable
 *
 * One TuiClass exists for each control type.
 */
struct TuiClass {
    void (*draw)(TuiControl *control, TuiDraw *draw);
    int  (*event)(TuiControl *control, TuiEvent *event);
};


/*
 * Base control.
 *
 * This MUST be the first member of every derived control.
 */
struct TuiControl {
    const TuiClass *cls;

    /*
     * Control tree.
     */
    TuiControl *parent;

    TuiControl *first;
    TuiControl *last;

    TuiControl *next;
    TuiControl *prev;

    /*
     * Position relative to parent's client area.
     */
    int x;
    int y;
    int width;
    int height;

    int flags;
};


/*
 * Desktop
 *
 * Root of the complete UI.
 *
 * Focus belongs to the desktop, not to individual windows.
 *
 * overlay is reserved for controls which must be rendered
 * above the normal control tree (menus, popup lists, etc.).
 */
struct TuiDesktop {
    TuiControl control;

    TuiControl *focused;
    TuiControl *capture;
};


/*
 * Window
 */
struct TuiWindow {
    TuiControl control;

    const char *title;
};


/*
 * Label
 */
struct TuiLabel {
    TuiControl control;

    const char *text;
};


/*
 * Button
 */
struct TuiButton {
    TuiControl control;

    const char *text;
    int command;
};


/*
 * Library
 */
int  tui_init(void);
void tui_shutdown(void);


/*
 * Control tree
 */
void tui_add(TuiControl *parent, TuiControl *child);

void tui_remove(TuiControl *control);

/*
 * Desktop
 */
void tui_desktop_init(TuiDesktop *desktop);

void tui_desktop_set_focus(TuiDesktop *desktop,
                           TuiControl *control);

TuiControl *tui_desktop_get_focus(TuiDesktop *desktop);

void tui_desktop_set_capture(TuiDesktop *desktop,
                             TuiControl *control);

void tui_desktop_clear_capture(TuiDesktop *desktop);


/*
 * Main UI operations
 */
void tui_draw(TuiDesktop *desktop);

int tui_dispatch(TuiDesktop *desktop,
                 TuiEvent *event);

int tui_read_event(TuiEvent *event);


/*
 * Window
 */
void tui_window_init(TuiWindow *window,
                     int x,
                     int y,
                     int width,
                     int height,
                     const char *title);


/*
 * Label
 */
void tui_label_init(TuiLabel *label,
                    int x,
                    int y,
                    const char *text);

void tui_label_set_text(TuiLabel *label,
                        const char *text);


/*
 * Button
 */
void tui_button_init(TuiButton *button,
                     int x,
                     int y,
                     int width,
                     const char *text,
                     int command);

/*
 * ------------------------------------------------------------
 * Menus
 * ------------------------------------------------------------
 */

#define TUI_MENU_SEPARATOR  0x0001
#define TUI_MENU_DISABLED   0x0002

typedef struct TuiMenuItem {
    const char *text;
    int command;
    int key;
    int flags;
} TuiMenuItem;

typedef struct TuiMenu {
    const char *text;

    TuiMenuItem *items;
    int count;
} TuiMenu;

typedef struct TuiPopupMenu {
    TuiControl control;

    struct TuiMenuBar *owner;

    TuiMenu *menu;
    int selected;
} TuiPopupMenu;

typedef struct TuiMenuBar {
    TuiControl control;

    TuiMenu *menus;
    int count;

    int selected;
    int active;

    TuiPopupMenu popup;
} TuiMenuBar;

/*
 * ------------------------------------------------------------
 * Status bar
 * ------------------------------------------------------------
 */

typedef struct TuiStatusItem {
    const char *text;
    int key;
    int command;
} TuiStatusItem;

typedef struct TuiStatusBar {
    TuiControl control;

    TuiStatusItem *items;
    int count;

    const char *status;
} TuiStatusBar;

#endif /* TUI_H */
