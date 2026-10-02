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

static const char *sb_items[] = {
    "Item 01", "Item 02", "Item 03", "Item 04", "Item 05",
    "Item 06", "Item 07", "Item 08", "Item 09", "Item 10",
    "Item 11", "Item 12", "Item 13", "Item 14", "Item 15",
    "Item 16", "Item 17", "Item 18", "Item 19", "Item 20"
};

static void sb_setup(TuiDesktop *desktop, TuiListBox *list,
                     int height, int count, int enabled)
{
    test_init_desktop(desktop);
    tui_listbox_init(list, 2, 2, 10, height, sb_items, count);
    tui_add(&desktop->control, &list->control);
    tui_listbox_set_scrollbar(list, enabled);
}

static int sb_visible(const TuiListBox *list)
{
    return (list->scrollbar.control.flags & TUI_VISIBLE) != 0;
}

static void test_scrollbar_default_off(void)
{
    TuiDesktop desktop;
    TuiListBox list;
    TuiEvent event;

    sb_setup(&desktop, &list, 5, 20, 0);
    tui_draw(&desktop);

    CHECK(!list.scrollbar_enabled);
    CHECK(!sb_visible(&list));
    CHECK(test_cell_chars[2][2] == 'I');
    CHECK(test_cell_chars[2][8] == '1');
    CHECK(test_cell_chars[2][11] == ' ');

    /* The last column still selects rows. */
    CHECK(test_mouse_action(&desktop, 11, 4, TUI_MOUSE_DOWN, &event));
    CHECK(tui_listbox_get_selected(&list) == 2);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(list.offset == 15);
}

static void test_scrollbar_visibility_and_geometry(void)
{
    TuiDesktop desktop;
    TuiListBox list;

    sb_setup(&desktop, &list, 5, 20, 1);
    CHECK(sb_visible(&list));
    CHECK(list.scrollbar.control.x == 9);
    CHECK(list.scrollbar.control.y == 0);
    CHECK(list.scrollbar.control.width == 1);
    CHECK(list.scrollbar.control.height == 5);
    CHECK(list.scrollbar.min == 0 && list.scrollbar.max == 20);
    CHECK(list.scrollbar.page == 5);
    CHECK(list.scrollbar.command == TUI_CMD_NONE);

    tui_draw(&desktop);
    CHECK(test_cell_chars[2][11] == TUI_CH_UP_TRIANGLE);
    CHECK(test_cell_chars[6][11] == TUI_CH_DOWN_TRIANGLE);
    CHECK(test_cell_chars[2][10] == ' ');
    CHECK(test_cell_chars[2][2] == 'I');

    /* Fits exactly: hidden and full width. */
    sb_setup(&desktop, &list, 5, 5, 1);
    CHECK(!sb_visible(&list));

    /* One more item than rows: visible, offsets 0 and 1 only. */
    sb_setup(&desktop, &list, 5, 6, 1);
    CHECK(sb_visible(&list));
    CHECK(test_key(&desktop, TUI_KEY_END) == 0);
    tui_desktop_set_focus(&desktop, &list.control);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(list.offset == 1);
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == 1);

    /* Few items: auto-hidden, full width drawn. */
    sb_setup(&desktop, &list, 5, 3, 1);
    CHECK(!sb_visible(&list));
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == 'I');
    CHECK(test_cell_chars[3][11] == ' ');

    /* Empty list. */
    sb_setup(&desktop, &list, 5, 20, 1);
    tui_listbox_set_items(&list, 0, 0);
    CHECK(!sb_visible(&list));
    CHECK(list.offset == 0);
    tui_draw(&desktop);

    /* Turning it off again restores everything. */
    sb_setup(&desktop, &list, 5, 20, 1);
    tui_listbox_set_scrollbar(&list, 0);
    CHECK(!sb_visible(&list));
    tui_listbox_set_scrollbar(0, 1);
}

static void test_scrollbar_sync_list_to_bar(void)
{
    TuiDesktop desktop;
    TuiListBox list;

    sb_setup(&desktop, &list, 5, 20, 1);
    tui_desktop_set_focus(&desktop, &list.control);

    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(list.offset == 15);
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == 15);
    CHECK(test_key(&desktop, TUI_KEY_PAGEUP));
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == list.offset);
    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == 0);

    tui_listbox_set_selected(&list, 12);
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == list.offset);
    CHECK(list.offset == 8);

    /* Items change: offset is normalized and the bar follows. */
    tui_listbox_set_items(&list, sb_items, 3);
    CHECK(!sb_visible(&list));
    CHECK(list.offset == 0);
    tui_listbox_set_items(&list, sb_items, 20);
    CHECK(sb_visible(&list));
}

static void test_scrollbar_mouse(void)
{
    TuiDesktop desktop;
    TuiListBox list;
    TuiEvent event;

    sb_setup(&desktop, &list, 5, 20, 1);
    tui_listbox_set_selected(&list, 1);

    /* Down arrow and track click move the viewport only. */
    CHECK(test_mouse_action(&desktop, 11, 6, TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(list.offset == 1);
    CHECK(tui_listbox_get_selected(&list) == 1);
    CHECK(desktop.focused == &list.control);

    CHECK(test_mouse_action(&desktop, 11, 5, TUI_MOUSE_DOWN, &event));
    CHECK(list.offset == 6);
    CHECK(tui_listbox_get_selected(&list) == 1);
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == 6);

    CHECK(test_mouse_action(&desktop, 11, 2, TUI_MOUSE_DOWN, &event));
    CHECK(list.offset == 5);

    /* Double click on the bar does not select or activate. */
    tui_listbox_set_command(&list, 77);
    CHECK(test_mouse_action(&desktop, 11, 4, TUI_MOUSE_DOUBLE, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(tui_listbox_get_selected(&list) == 1);

    /* A click in the content still selects. */
    CHECK(test_mouse_action(&desktop, 4, 3, TUI_MOUSE_DOWN, &event));
    CHECK(tui_listbox_get_selected(&list) == list.offset + 1);
}

static void test_scrollbar_drag_and_capture(void)
{
    TuiDesktop desktop;
    TuiListBox list;
    TuiEvent event;

    sb_setup(&desktop, &list, 5, 20, 1);

    /* Thumb is row 3; drag it past the bottom, outside the list. */
    CHECK(test_mouse_action(&desktop, 11, 3, TUI_MOUSE_DOWN, &event));
    CHECK(desktop.capture == &list.scrollbar.control);
    CHECK(desktop.focused == &list.control);
    CHECK(tui_listbox_get_selected(&list) == 0);

    CHECK(test_mouse_action(&desktop, 40, 20, TUI_MOUSE_MOVE, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(list.offset == 15);
    CHECK(tui_listbox_get_selected(&list) == 0);

    CHECK(test_mouse_action(&desktop, 40, 20, TUI_MOUSE_UP, &event));
    CHECK(desktop.capture == 0);
    CHECK(desktop.focused == &list.control);
}

static void test_scrollbar_resize(void)
{
    TuiDesktop desktop;
    TuiListBox list;

    sb_setup(&desktop, &list, 5, 20, 1);

    list.control.height = 8;
    tui_draw(&desktop);
    CHECK(list.scrollbar.page == 8);
    CHECK(list.scrollbar.control.height == 8);

    list.control.height = 20;
    tui_draw(&desktop);
    CHECK(!sb_visible(&list));
    CHECK(list.offset == 0);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][11] == ' ');

    list.control.height = 4;
    tui_draw(&desktop);
    CHECK(sb_visible(&list));
    CHECK(list.scrollbar.page == 4);

    tui_listbox_set_selected(&list, 19);
    list.control.height = 18;
    tui_draw(&desktop);
    CHECK(list.offset <= 2);
    CHECK(tui_scrollbar_get_value(&list.scrollbar) == list.offset);
}

static void test_scrollbar_focus_and_commands(void)
{
    TuiDesktop desktop;
    TuiListBox list;
    TuiListBox other;
    TuiEvent event;

    sb_setup(&desktop, &list, 5, 20, 1);
    tui_listbox_init(&other, 20, 2, 10, 5, sb_items, 20);
    tui_add(&desktop.control, &other.control);
    tui_listbox_set_command(&list, 77);

    CHECK(!(list.scrollbar.control.flags & TUI_FOCUSABLE));
    CHECK(!(list.scrollbar.control.flags & TUI_TABSTOP));

    tui_desktop_set_focus(&desktop, &list.control);
    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(desktop.focused == &other.control);
    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(desktop.focused == &list.control);

    /* Scrolling never reaches the application as a command. */
    CHECK(test_mouse_action(&desktop, 11, 6, TUI_MOUSE_DOWN, &event));
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(test_mouse_action(&desktop, 11, 3, TUI_MOUSE_DOWN, &event));
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(test_mouse_action(&desktop, 11, 5, TUI_MOUSE_MOVE, &event) >= 0);
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(test_mouse_action(&desktop, 11, 5, TUI_MOUSE_UP, &event) >= 0);
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(desktop.focused == &list.control);
}

void test_listbox_suite(void)
{
    test_run_case("listbox initial and empty", test_initial_and_empty);
    test_run_case("listbox keyboard and bounds", test_keyboard_and_bounds);
    test_run_case("listbox viewport and pages", test_viewport_and_pages);
    test_run_case("listbox mouse and drawing", test_mouse_and_drawing);
    test_run_case("listbox activation", test_activation);
    test_run_case("listbox type to select", test_type_to_select);
    test_run_case("listbox scrollbar default off",
                  test_scrollbar_default_off);
    test_run_case("listbox scrollbar visibility and geometry",
                  test_scrollbar_visibility_and_geometry);
    test_run_case("listbox scrollbar sync from list",
                  test_scrollbar_sync_list_to_bar);
    test_run_case("listbox scrollbar mouse", test_scrollbar_mouse);
    test_run_case("listbox scrollbar drag and capture",
                  test_scrollbar_drag_and_capture);
    test_run_case("listbox scrollbar resize", test_scrollbar_resize);
    test_run_case("listbox scrollbar focus and commands",
                  test_scrollbar_focus_and_commands);
}
