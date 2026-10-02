#include <stdio.h>

#include "test_support.h"

int test_checks;
int test_failures;
int test_cell_chars[TEST_HEIGHT][TEST_WIDTH];
int test_cell_attrs[TEST_HEIGHT][TEST_WIDTH];
int test_cursor_x;
int test_cursor_y;
int test_cursor_visible;

void test_check(int condition,
                const char *expression,
                const char *file,
                int line)
{
    ++test_checks;

    if (!condition) {
        ++test_failures;
        printf("%s:%d: check failed: %s\n",
               file, line, expression);
    }
}

void test_run_case(const char *name, TestFn function)
{
    int failures_before;

    failures_before = test_failures;
    function();

    if (failures_before == test_failures)
        printf("PASS: %s\n", name);
    else
        printf("FAIL: %s\n", name);

    fflush(stdout);
}

int tui_console_init(void)
{
    return 1;
}

void tui_console_shutdown(void)
{
}

int tui_console_width(void)
{
    return TEST_WIDTH;
}

int tui_console_height(void)
{
    return TEST_HEIGHT;
}

void tui_console_cell(int x, int y, int ch, int attr)
{
    if (x < 0 || x >= TEST_WIDTH ||
        y < 0 || y >= TEST_HEIGHT)
        return;

    test_cell_chars[y][x] = ch;
    test_cell_attrs[y][x] = attr;
}

int tui_console_key(void)
{
    return TUI_KEY_NONE;
}

void tui_console_mouse(int *x, int *y, int *action, int *buttons)
{
    *x = 0;
    *y = 0;
    *action = 0;
    *buttons = 0;
}

void tui_console_cursor(int x, int y, int visible)
{
    test_cursor_x = x;
    test_cursor_y = y;
    test_cursor_visible = visible;
}

void tui_console_present(void)
{
}

void test_reset_screen(void)
{
    int x;
    int y;

    for (y = 0; y < TEST_HEIGHT; ++y) {
        for (x = 0; x < TEST_WIDTH; ++x) {
            test_cell_chars[y][x] = -1;
            test_cell_attrs[y][x] = -1;
        }
    }

    test_cursor_x = -1;
    test_cursor_y = -1;
    test_cursor_visible = 0;
}

void test_init_desktop(TuiDesktop *desktop)
{
    tui_desktop_init(desktop);
    test_reset_screen();
}

int test_key(TuiDesktop *desktop, int key)
{
    TuiEvent event;

    event.type = TUI_EV_KEY;
    event.key = key;
    event.command = TUI_CMD_NONE;
    event.mouse_x = 0;
    event.mouse_y = 0;
    event.mouse_action = 0;
    event.mouse_buttons = 0;
    event.source = 0;

    return tui_dispatch(desktop, &event);
}

int test_key_event(TuiDesktop *desktop,
                   int key,
                   TuiEvent *event)
{
    event->type = TUI_EV_KEY;
    event->key = key;
    event->command = TUI_CMD_NONE;
    event->mouse_x = 0;
    event->mouse_y = 0;
    event->mouse_action = 0;
    event->mouse_buttons = 0;
    event->source = 0;

    return tui_dispatch(desktop, event);
}

int test_mouse_action(TuiDesktop *desktop,
                     int x,
                     int y,
                     int action,
                     TuiEvent *event)
{
    event->type = TUI_EV_MOUSE;
    event->key = TUI_KEY_NONE;
    event->command = TUI_CMD_NONE;
    event->mouse_x = x;
    event->mouse_y = y;
    event->mouse_action = action;
    event->mouse_buttons =
        action == TUI_MOUSE_MOVE ? 0 : TUI_MOUSE_LEFT;
    event->source = 0;

    return tui_dispatch(desktop, event);
}

int test_mouse_down(TuiDesktop *desktop, int x, int y)
{
    TuiEvent event;

    event.type = TUI_EV_MOUSE;
    event.key = TUI_KEY_NONE;
    event.command = TUI_CMD_NONE;
    event.mouse_x = x;
    event.mouse_y = y;
    event.mouse_action = TUI_MOUSE_DOWN;
    event.mouse_buttons = TUI_MOUSE_LEFT;
    event.source = 0;

    return tui_dispatch(desktop, &event);
}
