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
typedef struct TuiEdit TuiEdit;
typedef struct TuiListBox TuiListBox;
typedef struct TuiCheckBox TuiCheckBox;
typedef struct TuiRadioButton TuiRadioButton;
typedef struct TuiComboBox TuiComboBox;
typedef struct TuiScrollBar TuiScrollBar;

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
#define TUI_EV_MOUSE    3


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

/* Control attr value meaning "use the parent's attribute". */
#define TUI_ATTR_INHERIT        (-1)

/*
 * Control docking.
 */
#define TUI_DOCK_NONE    0
#define TUI_DOCK_TOP     1
#define TUI_DOCK_BOTTOM  2
#define TUI_DOCK_LEFT    3
#define TUI_DOCK_RIGHT   4
#define TUI_DOCK_FILL    5

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
    /* Desktop being drawn; receives cursor requests. */
    TuiDesktop *desktop;

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

    /*
     * Mouse: x/y are global screen coordinates.
     * mouse_buttons is the button involved in a mouse action
     * (TUI_MOUSE_*); it is 0 for MOVE.
     */
    int mouse_x;
    int mouse_y;
    int mouse_action;
    int mouse_buttons;

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
    
    int dock;

    int flags;

    /* Explicit attribute or TUI_ATTR_INHERIT. */
    int attr;
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

    /* Cursor requested during the last draw, in screen coordinates. */
    int cursor_visible;
    int cursor_x;
    int cursor_y;
};


/*
 * Window flags. flags == 0: movable, single border, centered title.
 * Title alignment bits: 0 = center; the unused 0x000C is treated
 * as center.
 */
#define TUI_WINDOW_FIXED          0x0001
#define TUI_WINDOW_ACTIVE_DOUBLE  0x0002

#define TUI_WINDOW_TITLE_CENTER   0x0000
#define TUI_WINDOW_TITLE_LEFT     0x0004
#define TUI_WINDOW_TITLE_RIGHT    0x0008
#define TUI_WINDOW_TITLE_MASK     0x000C

/*
 * Window
 */
struct TuiWindow {
    TuiControl control;

    const char *title;

    /* Drag state; drag_dx/dy is the mouse offset inside the window. */
    int dragging;
    int drag_dx;
    int drag_dy;

    /* Title attribute; TUI_ATTR_INHERIT uses the window's attribute. */
    int title_attr;

    unsigned flags;
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

    /* Set by mouse DOWN; command fires on UP inside the button. */
    int pressed;
};

/*
 * Scroll bar
 *
 * Model: min/max is the whole logical range, page is the visible
 * amount and value is the first visible position. The largest
 * effective value is max - page (or min when page >= max - min).
 * The setters keep the state normalized and never emit commands;
 * only user interaction turns a changed value into a command.
 */
#define TUI_VERTICAL    0
#define TUI_HORIZONTAL  1

struct TuiScrollBar {
    TuiControl control;

    int min;
    int max;
    int value;
    int page;

    int orientation;
    int command;

    /* Thumb drag; drag_offset is the grabbed cell inside the thumb. */
    int dragging;
    int drag_offset;
};

/*
 * Check box
 */
struct TuiCheckBox {
    TuiControl control;

    const char *text;
    int checked;
    int command;
};

/*
 * Radio button
 */
struct TuiRadioButton {
    TuiControl control;

    const char *text;
    int group;
    int checked;
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

/* Effective attribute: walks up the parents until one is not INHERIT. */
int tui_control_attr(TuiControl *control);

void tui_remove(TuiControl *control);

/* Moves control to the end of its parent's list (top of z-order). */
void tui_bring_to_front(TuiControl *control);

/* Deepest visible control under screen (x,y), or 0. */
TuiControl *tui_hit_test(TuiControl *root, int x, int y);

/* Converts screen coordinates to the control's local coordinates. */
void tui_control_screen_to_local(TuiControl *control,
                                 int screen_x, int screen_y,
                                 int *local_x, int *local_y);

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

/* Request the cursor at local (x,y); ignored if clipped. */
void tui_draw_cursor(TuiDraw *draw, int x, int y);

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
void tui_scrollbar_init(TuiScrollBar *scrollbar,
                        int x,
                        int y,
                        int length,
                        int orientation,
                        int command);
void tui_scrollbar_set_range(TuiScrollBar *scrollbar,
                             int min,
                             int max);
void tui_scrollbar_set_page(TuiScrollBar *scrollbar, int page);
void tui_scrollbar_set_value(TuiScrollBar *scrollbar, int value);
int tui_scrollbar_get_value(const TuiScrollBar *scrollbar);
void tui_scrollbar_set_command(TuiScrollBar *scrollbar, int command);

void tui_window_set_flags(TuiWindow *window, unsigned flags);
unsigned tui_window_get_flags(const TuiWindow *window);
void tui_window_add_flags(TuiWindow *window, unsigned flags);
void tui_window_remove_flags(TuiWindow *window, unsigned flags);


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
 * Check box
 */
void tui_checkbox_init(TuiCheckBox *checkbox,
                       int x,
                       int y,
                       int width,
                       const char *text);

void tui_checkbox_set_checked(TuiCheckBox *checkbox, int checked);
int tui_checkbox_get_checked(TuiCheckBox *checkbox);
void tui_checkbox_set_command(TuiCheckBox *checkbox, int command);

/*
 * Radio button
 */
void tui_radiobutton_init(TuiRadioButton *radio,
                          int x,
                          int y,
                          int width,
                          const char *text,
                          int group);

void tui_radiobutton_set_checked(TuiRadioButton *radio, int checked);
int tui_radiobutton_get_checked(TuiRadioButton *radio);
void tui_radiobutton_set_command(TuiRadioButton *radio, int command);

/*
 * Combo box
 */
void tui_combobox_init(TuiComboBox *combo,
                       int x,
                       int y,
                       int width,
                       const char **items,
                       int count);

void tui_combobox_set_items(TuiComboBox *combo,
                            const char **items,
                            int count);

int tui_combobox_get_selected(TuiComboBox *combo);
void tui_combobox_set_selected(TuiComboBox *combo, int index);
/* Scroll bar in the popup list when the items do not fit. */
void tui_combobox_set_scrollbar(TuiComboBox *combo, int enabled);
void tui_combobox_set_command(TuiComboBox *combo, int command);

/*
 * Edit
 */
void tui_edit_init(TuiEdit *edit,
                   int x,
                   int y,
                   int width,
                   char *buffer,
                   int capacity);

void tui_edit_set_text(TuiEdit *edit,
                       const char *text);

const char *tui_edit_get_text(TuiEdit *edit);

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

void tui_menubar_init(TuiMenuBar *bar,
                      TuiMenu *menus,
                      int count);

void tui_menubar_activate(TuiMenuBar *bar);

void tui_statusbar_init(TuiStatusBar *bar,
                        TuiStatusItem *items,
                        int count);

void tui_statusbar_set_text(TuiStatusBar *bar,
                            const char *text);

/*
 * ------------------------------------------------------------
 * Edit
 * ------------------------------------------------------------
 */

struct TuiEdit {
    TuiControl control;

    char *text;
    int capacity;

    int length;
    int cursor;
    int offset;
};

/*
 * List box
 */
struct TuiListBox {
    TuiControl control;

    const char **items;
    int count;

    int selected;
    int offset;
    int command;

    /* Optional internal vertical scroll bar, a hidden child. */
    TuiScrollBar scrollbar;
    int scrollbar_enabled;
};

/*
 * Combo box
 */
struct TuiComboBox {
    TuiControl control;

    const char **items;
    int count;
    int selected;
    int command;

    int open;
    int original_selected;

    TuiWindow popup_window;
    TuiListBox popup_list;
};

void tui_listbox_init(TuiListBox *list,
                      int x,
                      int y,
                      int width,
                      int height,
                      const char **items,
                      int count);

void tui_listbox_set_items(TuiListBox *list,
                           const char **items,
                           int count);

int tui_listbox_get_selected(TuiListBox *list);

void tui_listbox_set_selected(TuiListBox *list,
                              int index);

/*
 * Reserves a column for an internal scroll bar when the items do not
 * fit. Disabled by default.
 */
void tui_listbox_set_scrollbar(TuiListBox *list, int enabled);
void tui_listbox_set_command(TuiListBox *list,
                             int command);

#endif /* TUI_H */
