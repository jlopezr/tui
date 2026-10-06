#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include "tui.h"

#define TEST_WIDTH  80
#define TEST_HEIGHT 25

typedef void (*TestFn)(void);

extern int test_checks;
extern int test_failures;
extern int test_cell_chars[TEST_HEIGHT][TEST_WIDTH];
extern int test_cell_attrs[TEST_HEIGHT][TEST_WIDTH];
extern int test_cursor_x;
extern int test_cursor_y;
extern int test_cursor_visible;
extern int test_console_8bit;
extern int test_console_mouse;

void test_check(int condition,
                const char *expression,
                const char *file,
                int line);
void test_run_case(const char *name, TestFn function);
void test_reset_screen(void);
void test_init_desktop(TuiDesktop *desktop);
int test_key(TuiDesktop *desktop, int key);

/* Cells the code under test has written to the console so far. */
extern int test_cells_written;

/*
 * Writes that changed the cell they wrote to. The MiniCPU console skips the others
 * (it compares with a copy of the screen), so these are the ones that reach the
 * text RAM.
 */
extern int test_cells_changed;

/*
 * Counts the writes that change a cell outside a rectangle (x2, y2 exclusive),
 * from test_watch_outside() until test_watch_off().
 */
extern int test_watch_writes;
void test_watch_outside(int x1, int y1, int x2, int y2);
void test_watch_off(void);
int test_key_event(TuiDesktop *desktop, int key, TuiEvent *event);
int test_mouse_action(TuiDesktop *desktop,
                      int x,
                      int y,
                      int action,
                      TuiEvent *event);
int test_mouse_down(TuiDesktop *desktop, int x, int y);

/*
 * The console's input queue, for tests of tui_read_event(). The fake console hands
 * out what was pushed, in order, and has nothing (TUI_KEY_NONE) when it runs dry:
 * it never waits. Like a real console, the details of a mouse event are only good
 * until the next one is taken.
 */
void test_console_clear_input(void);
void test_console_push_key(int key);
void test_console_push_mouse(int x, int y, int action, int buttons);
int test_console_input_left(void);

#define CHECK(expression) \
    test_check((expression), #expression, __FILE__, __LINE__)

void test_listbox_suite(void);
void test_edit_suite(void);
void test_button_suite(void);
void test_window_suite(void);
void test_menu_suite(void);
void test_statusbar_suite(void);
void test_label_suite(void);
void test_core_suite(void);
void test_checkbox_suite(void);
void test_radiobutton_suite(void);
void test_combobox_suite(void);
void test_scrollbar_suite(void);
void test_editor_buffer_suite(void);
void test_textmodel_suite(void);
void test_editor_suite(void);
void test_mini_keys_suite(void);

#endif
