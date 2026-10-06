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

int test_cells_written;

static int test_watch_active;
static int test_watch_x1;
static int test_watch_y1;
static int test_watch_x2;
static int test_watch_y2;
int test_watch_writes;

void test_watch_outside(int x1, int y1, int x2, int y2)
{
    test_watch_active = 1;
    test_watch_x1 = x1;
    test_watch_y1 = y1;
    test_watch_x2 = x2;
    test_watch_y2 = y2;
    test_watch_writes = 0;
}

void test_watch_off(void)
{
    test_watch_active = 0;
}

void tui_console_cell(int x, int y, int ch, int attr)
{
    if (x < 0 || x >= TEST_WIDTH ||
        y < 0 || y >= TEST_HEIGHT)
        return;

    ++test_cells_written;

    /*
     * A write that changes the cell is something the screen shows, even if a later
     * one puts the old value back (the MiniCPU console writes straight to the text
     * RAM, and skips only the writes that change nothing).
     */
    if (test_watch_active &&
        (x < test_watch_x1 || x >= test_watch_x2 ||
         y < test_watch_y1 || y >= test_watch_y2) &&
        (test_cell_chars[y][x] != ch || test_cell_attrs[y][x] != attr))
        ++test_watch_writes;

    test_cell_chars[y][x] = ch;
    test_cell_attrs[y][x] = attr;
}

#define TEST_INPUT_MAX 64

typedef struct TestInput {
    int key;
    int x;
    int y;
    int action;
    int buttons;
} TestInput;

static TestInput test_input[TEST_INPUT_MAX];
static int test_input_head;
static int test_input_tail;
static int test_mouse_x;
static int test_mouse_y;
static int test_mouse_action_now;
static int test_mouse_buttons;

void test_console_clear_input(void)
{
    test_input_head = 0;
    test_input_tail = 0;
}

void test_console_push_key(int key)
{
    test_input[test_input_tail].key = key;
    ++test_input_tail;
}

void test_console_push_mouse(int x, int y, int action, int buttons)
{
    test_input[test_input_tail].key = TUI_KEY_MOUSE;
    test_input[test_input_tail].x = x;
    test_input[test_input_tail].y = y;
    test_input[test_input_tail].action = action;
    test_input[test_input_tail].buttons = buttons;
    ++test_input_tail;
}

int test_console_input_left(void)
{
    return test_input_tail - test_input_head;
}

int tui_console_poll(void)
{
    TestInput *input;

    if (test_input_head == test_input_tail)
        return TUI_KEY_NONE;

    input = &test_input[test_input_head++];

    if (input->key == TUI_KEY_MOUSE) {
        test_mouse_x = input->x;
        test_mouse_y = input->y;
        test_mouse_action_now = input->action;
        test_mouse_buttons = input->buttons;
    }

    return input->key;
}

int tui_console_key(void)
{
    return tui_console_poll();
}

void tui_console_mouse(int *x, int *y, int *action, int *buttons)
{
    *x = test_mouse_x;
    *y = test_mouse_y;
    *action = test_mouse_action_now;
    *buttons = test_mouse_buttons;
}

int test_console_8bit = 0;
int test_console_mouse = 1;

int tui_console_has_mouse(void)
{
    return test_console_mouse;
}

int tui_console_printable(int key)
{
    if (TUI_ASCII_PRINTABLE(key))
        return 1;
    return test_console_8bit && key >= 128 && key <= 255;
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
