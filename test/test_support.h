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
int test_key_event(TuiDesktop *desktop, int key, TuiEvent *event);
int test_mouse_action(TuiDesktop *desktop,
                      int x,
                      int y,
                      int action,
                      TuiEvent *event);
int test_mouse_down(TuiDesktop *desktop, int x, int y);

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
void test_textarea_suite(void);
void test_textmodel_suite(void);
void test_editor_suite(void);
void test_mini_keys_suite(void);

#endif
