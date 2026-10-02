#include <stdio.h>
#include <string.h>

#include "tui.h"

#define TEST_WIDTH  80
#define TEST_HEIGHT 25

typedef void (*TestFn)(void);

typedef struct TestCase {
    const char *name;
    TestFn function;
} TestCase;

static int test_checks;
static int test_failures;
static int test_cell_chars[TEST_HEIGHT][TEST_WIDTH];
static int test_cell_attrs[TEST_HEIGHT][TEST_WIDTH];
static int test_cursor_x;
static int test_cursor_y;
static int test_cursor_visible;

static void test_check(int condition,
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

#define CHECK(expression) \
    test_check((expression), #expression, __FILE__, __LINE__)

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

static void test_reset_screen(void)
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

static void test_init_desktop(TuiDesktop *desktop)
{
    tui_desktop_init(desktop);
    test_reset_screen();
}

static int test_key(TuiDesktop *desktop, int key)
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

static int test_key_event(TuiDesktop *desktop,
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

static int test_mouse_action(TuiDesktop *desktop,
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
    event->mouse_buttons = TUI_MOUSE_LEFT;
    event->source = 0;

    return tui_dispatch(desktop, event);
}

static int test_mouse_down(TuiDesktop *desktop, int x, int y)
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

static void test_listbox_initial_and_empty(void)
{
    static const char *items[] = { "Alpha", "Beta" };
    TuiListBox list;

    tui_listbox_init(&list, 0, 0, 8, 2, items, 2);
    CHECK(list.items == items);
    CHECK(list.count == 2);
    CHECK(tui_listbox_get_selected(&list) == 0);
    CHECK(list.offset == 0);

    tui_listbox_set_items(&list, 0, 0);
    CHECK(list.items == 0);
    CHECK(list.count == 0);
    CHECK(tui_listbox_get_selected(&list) == -1);
    CHECK(list.offset == 0);

    tui_listbox_init(&list, 0, 0, 8, 2, items, 0);
    CHECK(list.items == 0);
    CHECK(list.count == 0);
    CHECK(tui_listbox_get_selected(&list) == -1);
    CHECK(list.offset == 0);
}

static void test_listbox_keyboard_and_bounds(void)
{
    static const char *items[] = { "Alpha", "Beta", "Gamma" };
    TuiDesktop desktop;
    TuiListBox list;

    test_init_desktop(&desktop);
    tui_listbox_init(&list, 1, 1, 8, 2, items, 3);
    tui_add(&desktop.control, &list.control);
    tui_desktop_set_focus(&desktop, &list.control);

    CHECK(test_key(&desktop, TUI_KEY_UP));
    CHECK(tui_listbox_get_selected(&list) == 0);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(tui_listbox_get_selected(&list) == 2);
    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(tui_listbox_get_selected(&list) == 0);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(tui_listbox_get_selected(&list) == 2);

    tui_listbox_set_items(&list, 0, 0);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(tui_listbox_get_selected(&list) == -1);
}

static void test_listbox_viewport_and_pages(void)
{
    static const char *items[] = {
        "Apple", "Banana", "Orange", "Peach", "Pear", "Mango"
    };
    TuiDesktop desktop;
    TuiListBox list;

    test_init_desktop(&desktop);
    tui_listbox_init(&list, 1, 1, 8, 4, items, 6);
    tui_add(&desktop.control, &list.control);
    tui_desktop_set_focus(&desktop, &list.control);

    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(list.selected == 4);
    CHECK(list.offset == 1);

    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(list.selected == 0);
    CHECK(list.offset == 0);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(list.selected == 5);
    CHECK(list.offset == 2);
    CHECK(test_key(&desktop, TUI_KEY_PAGEUP));
    CHECK(list.selected == 1);
    CHECK(test_key(&desktop, TUI_KEY_PAGEDOWN));
    CHECK(list.selected == 5);

    tui_listbox_set_selected(&list, -1);
    CHECK(list.selected == 0);
    CHECK(list.offset == 0);
    tui_listbox_set_selected(&list, 99);
    CHECK(list.selected == 5);
    CHECK(list.offset == 2);
}

static void test_listbox_mouse_and_drawing(void)
{
    static const char *items[] = { "Apple", "Banana", "Orange" };
    TuiDesktop desktop;
    TuiListBox list;

    test_init_desktop(&desktop);
    tui_listbox_init(&list, 2, 3, 4, 2, items, 3);
    tui_add(&desktop.control, &list.control);

    CHECK(test_mouse_down(&desktop, 3, 4));
    CHECK(desktop.focused == &list.control);
    CHECK(list.selected == 1);

    tui_draw(&desktop);
    CHECK(test_cell_chars[3][2] == 'A');
    CHECK(test_cell_chars[3][5] == 'l');
    CHECK(test_cell_chars[4][2] == 'B');
    CHECK(test_cell_chars[4][5] == 'a');
    CHECK(test_cell_chars[4][6] == ' ');
    CHECK(test_cell_attrs[4][2] == TUI_ATTR_MENU_SELECTED);

    tui_listbox_set_selected(&list, 2);
    tui_draw(&desktop);
    CHECK(test_cell_chars[3][2] == 'B');
    CHECK(test_cell_chars[4][2] == 'O');
    CHECK(test_cell_attrs[3][2] == tui_control_attr(&list.control));
}

static void test_listbox_activation(void)
{
    static const char *items[] = { "Apple", "Banana", "Orange", "Peach" };
    TuiDesktop desktop;
    TuiListBox list;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_listbox_init(&list, 2, 3, 12, 5, items, 4);
    tui_add(&desktop.control, &list.control);
    tui_listbox_set_command(&list, 42);

    CHECK(test_mouse_down(&desktop, 3, 4));
    CHECK(list.selected == 1);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 42);
    CHECK(event.source == &list.control);
    CHECK(list.selected == 1);

    CHECK(test_mouse_action(&desktop, 3, 5,
                            TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(list.selected == 2);

    CHECK(test_mouse_action(&desktop, 3, 6,
                            TUI_MOUSE_DOUBLE, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 42);
    CHECK(event.source == &list.control);
    CHECK(list.selected == 3);

    CHECK(test_mouse_action(&desktop, 3, 7,
                            TUI_MOUSE_DOUBLE, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(list.selected == 3);

    tui_listbox_set_command(&list, TUI_CMD_NONE);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_KEY);
    CHECK(test_mouse_action(&desktop, 3, 4,
                            TUI_MOUSE_DOUBLE, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(list.selected == 1);

    tui_listbox_set_command(&list, 42);
    tui_listbox_set_items(&list, 0, 0);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_KEY);
    CHECK(test_mouse_action(&desktop, 3, 4,
                            TUI_MOUSE_DOUBLE, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(test_key_event(&desktop, 'A', &event));
    CHECK(event.type == TUI_EV_KEY);
    CHECK(list.selected == -1);

    tui_listbox_set_items(&list, items, 4);
    list.selected = -1;
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_KEY);

    list.selected = list.count;
    CHECK(test_mouse_action(&desktop, 3, 7,
                            TUI_MOUSE_DOUBLE, &event));
    CHECK(event.type == TUI_EV_MOUSE);
}

static void test_listbox_type_to_select(void)
{
    static const char *items[] = {
        "Apple", "Banana", "Mango", "Orange", "Peach", "Pear"
    };
    static const char *same_letter[] = {
        "Alpha", "Apricot", "", "Avocado", "1st", "2nd", 0
    };
    TuiDesktop desktop;
    TuiListBox list;

    test_init_desktop(&desktop);
    tui_listbox_init(&list, 1, 1, 10, 2, items, 6);
    tui_add(&desktop.control, &list.control);
    tui_desktop_set_focus(&desktop, &list.control);

    CHECK(test_key(&desktop, 'm'));
    CHECK(list.selected == 2);
    CHECK(list.offset == 1);
    CHECK(test_key(&desktop, 'P'));
    CHECK(list.selected == 4);
    CHECK(test_key(&desktop, 'p'));
    CHECK(list.selected == 5);
    CHECK(test_key(&desktop, 'P'));
    CHECK(list.selected == 4);
    CHECK(test_key(&desktop, 'z'));
    CHECK(list.selected == 4);
    CHECK(!test_key(&desktop, '!'));

    tui_listbox_set_items(&list, same_letter, 6);
    CHECK(test_key(&desktop, 'a'));
    CHECK(list.selected == 1);
    CHECK(test_key(&desktop, 'A'));
    CHECK(list.selected == 3);
    CHECK(test_key(&desktop, 'a'));
    CHECK(list.selected == 0);
    CHECK(test_key(&desktop, '2'));
    CHECK(list.selected == 5);
}

static void test_edit_text_and_editing_keys(void)
{
    char buffer[8];
    TuiDesktop desktop;
    TuiEdit edit;

    buffer[0] = '\0';
    test_init_desktop(&desktop);
    tui_edit_init(&edit, 1, 1, 6, buffer, 8);
    tui_add(&desktop.control, &edit.control);
    tui_desktop_set_focus(&desktop, &edit.control);

    tui_edit_set_text(&edit, "abcd");
    CHECK(strcmp(tui_edit_get_text(&edit), "abcd") == 0);
    CHECK(edit.length == 4);
    CHECK(edit.cursor == 4);

    CHECK(test_key(&desktop, TUI_KEY_LEFT));
    CHECK(test_key(&desktop, 'X'));
    CHECK(strcmp(tui_edit_get_text(&edit), "abcXd") == 0);
    CHECK(edit.cursor == 4);

    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(test_key(&desktop, TUI_KEY_DELETE));
    CHECK(strcmp(tui_edit_get_text(&edit), "bcXd") == 0);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(test_key(&desktop, TUI_KEY_BACKSPACE));
    CHECK(strcmp(tui_edit_get_text(&edit), "bcX") == 0);
}

static void test_edit_capacity_set_text_and_mouse(void)
{
    char buffer[5];
    TuiDesktop desktop;
    TuiEdit edit;

    buffer[0] = '\0';
    test_init_desktop(&desktop);
    tui_edit_init(&edit, 2, 3, 3, buffer, 5);
    tui_add(&desktop.control, &edit.control);
    tui_desktop_set_focus(&desktop, &edit.control);

    tui_edit_set_text(&edit, "123456789");
    CHECK(strcmp(tui_edit_get_text(&edit), "1234") == 0);
    CHECK(edit.length == 4);
    CHECK(edit.cursor == 4);

    CHECK(test_key(&desktop, '5'));
    CHECK(strcmp(tui_edit_get_text(&edit), "1234") == 0);

    tui_edit_set_text(&edit, "abcd");
    CHECK(test_mouse_down(&desktop, 4, 3));
    CHECK(desktop.focused == &edit.control);
    CHECK(edit.cursor == 2);

    tui_edit_set_text(&edit, 0);
    CHECK(strcmp(tui_edit_get_text(&edit), "") == 0);
    CHECK(edit.length == 0);
    CHECK(edit.cursor == 0);
}

static void test_edit_horizontal_viewport_and_drawing(void)
{
    char buffer[16];
    TuiDesktop desktop;
    TuiEdit edit;

    buffer[0] = '\0';
    test_init_desktop(&desktop);
    tui_edit_init(&edit, 2, 2, 3, buffer, 16);
    tui_add(&desktop.control, &edit.control);
    tui_desktop_set_focus(&desktop, &edit.control);

    tui_edit_set_text(&edit, "abcdefgh");
    CHECK(test_key(&desktop, TUI_KEY_LEFT));
    CHECK(edit.cursor == 7);
    CHECK(edit.offset == 5);

    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == 'f');
    CHECK(test_cell_chars[2][3] == 'g');
    CHECK(test_cell_chars[2][4] == 'h');
    CHECK(test_cursor_visible);
    CHECK(test_cursor_x == 4);
    CHECK(test_cursor_y == 2);

    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(edit.cursor == 0);
    CHECK(edit.offset == 0);
}

static const TestCase test_cases[] = {
    { "listbox initial and empty", test_listbox_initial_and_empty },
    { "listbox keyboard and bounds", test_listbox_keyboard_and_bounds },
    { "listbox viewport and pages", test_listbox_viewport_and_pages },
    { "listbox mouse and drawing", test_listbox_mouse_and_drawing },
    { "listbox activation", test_listbox_activation },
    { "listbox type to select", test_listbox_type_to_select },
    { "edit text and editing keys", test_edit_text_and_editing_keys },
    { "edit capacity and mouse", test_edit_capacity_set_text_and_mouse },
    { "edit horizontal viewport", test_edit_horizontal_viewport_and_drawing }
};

int main(void)
{
    int i;
    int failures_before;

    test_checks = 0;
    test_failures = 0;

    for (i = 0;
         i < (int)(sizeof(test_cases) / sizeof(test_cases[0]));
         ++i) {
        failures_before = test_failures;
        test_cases[i].function();

        if (test_failures == failures_before)
            printf("PASS: %s\n", test_cases[i].name);
        else
            printf("FAIL: %s\n", test_cases[i].name);
    }

    printf("%d checks, %d failures\n",
           test_checks,
           test_failures);

    return test_failures == 0 ? 0 : 1;
}
