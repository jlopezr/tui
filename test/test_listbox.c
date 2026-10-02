#include "test_support.h"

static void test_initial_and_empty(void)
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

static void test_keyboard_and_bounds(void)
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

static void test_viewport_and_pages(void)
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

static void test_mouse_and_drawing(void)
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

static void test_activation(void)
{
    static const char *items[] = {
        "Apple", "Banana", "Orange", "Peach"
    };
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

static void test_type_to_select(void)
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

void test_listbox_suite(void)
{
    test_run_case("listbox initial and empty", test_initial_and_empty);
    test_run_case("listbox keyboard and bounds", test_keyboard_and_bounds);
    test_run_case("listbox viewport and pages", test_viewport_and_pages);
    test_run_case("listbox mouse and drawing", test_mouse_and_drawing);
    test_run_case("listbox activation", test_activation);
    test_run_case("listbox type to select", test_type_to_select);
}
