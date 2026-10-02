#include "test_support.h"

static void test_empty_and_single_item(void)
{
    static const char *one_item[] = { "Only" };
    TuiDesktop desktop;
    TuiComboBox combo;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_combobox_init(&combo, 2, 2, 12, 0, 0);
    tui_add(&desktop.control, &combo.control);
    tui_desktop_set_focus(&desktop, &combo.control);

    CHECK(tui_combobox_get_selected(&combo) == -1);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(!combo.open);
    CHECK(event.type == TUI_EV_KEY);

    tui_combobox_set_items(&combo, one_item, 1);
    CHECK(tui_combobox_get_selected(&combo) == 0);
    CHECK(test_key_event(&desktop, ' ', &event));
    CHECK(combo.open);
    CHECK(test_key_event(&desktop, TUI_KEY_ESCAPE, &event));
}

static void test_keyboard_commit_and_cancel(void)
{
    static const char *items[] = {
        "BASIC", "Forth", "C", "Assembly"
    };
    TuiDesktop desktop;
    TuiComboBox combo;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_combobox_init(&combo, 5, 3, 16, items, 4);
    tui_combobox_set_command(&combo, 81);
    tui_add(&desktop.control, &combo.control);
    tui_desktop_set_focus(&desktop, &combo.control);

    CHECK(test_key_event(&desktop, ' ', &event));
    CHECK(combo.open);
    CHECK(desktop.capture == &combo.control);
    CHECK(combo.popup_window.control.parent == &desktop.control);
    CHECK(tui_combobox_get_selected(&combo) == 0);

    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(combo.popup_list.selected == 1);
    CHECK(tui_combobox_get_selected(&combo) == 0);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(combo.popup_list.selected == 3);
    CHECK(test_key(&desktop, TUI_KEY_HOME));
    CHECK(combo.popup_list.selected == 0);
    CHECK(test_key(&desktop, 'f'));
    CHECK(combo.popup_list.selected == 1);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(combo.popup_list.selected == 2);

    CHECK(test_key_event(&desktop, TUI_KEY_ESCAPE, &event));
    CHECK(!combo.open);
    CHECK(combo.popup_window.control.parent == 0);
    CHECK(desktop.capture == 0);
    CHECK(tui_desktop_get_focus(&desktop) == &combo.control);
    CHECK(tui_combobox_get_selected(&combo) == 0);
    CHECK(event.type == TUI_EV_KEY);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(combo.open);
    CHECK(test_key(&desktop, TUI_KEY_PAGEDOWN));
    CHECK(combo.popup_list.selected == 3);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(!combo.open);
    CHECK(tui_combobox_get_selected(&combo) == 3);
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 81);
    CHECK(event.source == &combo.control);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(combo.open);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_KEY);
}

static void test_mouse_selection_and_outside_cancel(void)
{
    static const char *items[] = { "One", "Two", "Three" };
    TuiDesktop desktop;
    TuiComboBox combo;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_combobox_init(&combo, 5, 3, 16, items, 3);
    tui_combobox_set_command(&combo, 82);
    tui_add(&desktop.control, &combo.control);

    CHECK(test_mouse_action(&desktop, 7, 3,
                            TUI_MOUSE_DOWN, &event));
    CHECK(combo.open);
    CHECK(tui_desktop_get_focus(&desktop) == &combo.control);
    CHECK(tui_hit_test(&desktop.control, 7, 5) ==
          &combo.popup_list.control);

    CHECK(test_mouse_action(&desktop, 7, 6,
                            TUI_MOUSE_DOWN, &event));
    CHECK(!combo.open);
    CHECK(tui_combobox_get_selected(&combo) == 1);
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 82);
    CHECK(event.source == &combo.control);
    CHECK(desktop.capture == 0);

    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(combo.open);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(combo.popup_list.selected == 2);
    CHECK(test_mouse_action(&desktop, 50, 20,
                            TUI_MOUSE_DOWN, &event));
    CHECK(!combo.open);
    CHECK(tui_combobox_get_selected(&combo) == 1);
    CHECK(event.type == TUI_EV_MOUSE);
}

static void test_viewport_width_and_drawing(void)
{
    static const char *items[] = {
        "None", "Size", "Speed", "Debug", "Maximum", "Fast",
        "Small", "Safe", "Trace", "Last"
    };
    TuiDesktop desktop;
    TuiComboBox combo;

    test_init_desktop(&desktop);
    tui_combobox_init(&combo, 2, 2, 10, items, 10);
    tui_combobox_set_selected(&combo, 1);
    tui_add(&desktop.control, &combo.control);
    tui_desktop_set_focus(&desktop, &combo.control);
    tui_draw(&desktop);

    CHECK(test_cell_chars[2][2] == '[');
    CHECK(test_cell_chars[2][3] == 'S');
    CHECK(test_cell_chars[2][10] == 'v');
    CHECK(test_cell_chars[2][11] == ']');

    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(combo.popup_list.control.height == 8);
    CHECK(combo.popup_window.control.height == 10);
    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(combo.popup_list.selected == 9);
    CHECK(combo.popup_list.offset == 2);
    CHECK(test_key(&desktop, TUI_KEY_PAGEUP));
    CHECK(combo.popup_list.selected == 1);
    CHECK(test_key(&desktop, TUI_KEY_PAGEDOWN));
    CHECK(combo.popup_list.selected == 9);
    CHECK(test_key(&desktop, TUI_KEY_ESCAPE));

    tui_combobox_set_items(&combo, 0, 0);
    CHECK(tui_combobox_get_selected(&combo) == -1);
    tui_remove(&combo.control);
    tui_combobox_init(&combo, 0, 4, 1, items, 10);
    tui_add(&desktop.control, &combo.control);
    tui_draw(&desktop);
    CHECK(test_cell_chars[4][0] == '[');
}

static void test_arrow_navigation_and_parent_attributes(void)
{
    static const char *items[] = { "First", "Second", "Third" };
    TuiDesktop desktop;
    TuiWindow parent;
    TuiComboBox combo;
    TuiEvent event;
    int parent_attr;

    parent_attr = TUI_ATTR(TUI_WHITE, TUI_RED);

    test_init_desktop(&desktop);
    tui_window_init(&parent, 1, 1, 20, 10, "Parent");
    parent.control.attr = parent_attr;
    tui_combobox_init(&combo, 1, 1, 12, items, 3);
    tui_add(&desktop.control, &parent.control);
    tui_add(&parent.control, &combo.control);
    tui_desktop_set_focus(&desktop, &combo.control);
    tui_draw(&desktop);

    CHECK(test_cell_chars[3][3] == '[');
    CHECK(test_cell_chars[3][4] == 'F');
    CHECK(test_cell_attrs[3][4] == parent_attr);

    CHECK(test_key_event(&desktop, TUI_KEY_DOWN, &event));
    CHECK(combo.open);
    CHECK(combo.popup_list.selected == 1);
    CHECK(tui_combobox_get_selected(&combo) == 0);
    CHECK(combo.popup_window.control.attr == parent_attr);
    tui_draw(&desktop);
    CHECK(test_cell_chars[combo.popup_window.control.y + 1][
              combo.popup_window.control.x + 1] == 'F');

    CHECK(test_key(&desktop, TUI_KEY_END));
    CHECK(combo.popup_list.selected == 2);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(combo.popup_list.selected == 2);
    CHECK(test_key(&desktop, TUI_KEY_UP));
    CHECK(combo.popup_list.selected == 1);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(!combo.open);
    CHECK(tui_combobox_get_selected(&combo) == 1);
}

static void test_popup_placement_above_and_setters(void)
{
    static const char *items[] = {
        "Alpha", "Beta", "Gamma", "Delta"
    };
    TuiDesktop desktop;
    TuiComboBox combo;

    test_init_desktop(&desktop);
    tui_combobox_init(&combo, 4, 23, 14, items, 4);
    tui_add(&desktop.control, &combo.control);
    tui_desktop_set_focus(&desktop, &combo.control);

    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(combo.open);
    CHECK(combo.popup_window.control.y < combo.control.y);
    CHECK(combo.popup_window.control.y >= 0);
    CHECK(combo.popup_window.control.y +
          combo.popup_window.control.height <= TEST_HEIGHT);

    tui_combobox_set_selected(&combo, 99);
    CHECK(tui_combobox_get_selected(&combo) == 3);
    CHECK(combo.popup_list.selected == 3);
    tui_combobox_set_items(&combo, items, 2);
    CHECK(!combo.open);
    CHECK(desktop.capture == 0);
    CHECK(tui_combobox_get_selected(&combo) == 0);

    tui_combobox_set_selected(&combo, -1);
    CHECK(tui_combobox_get_selected(&combo) == 0);
    tui_combobox_set_command(&combo, 0);
}

void test_combobox_suite(void)
{
    test_run_case("combobox empty and single item", test_empty_and_single_item);
    test_run_case("combobox keyboard commit and cancel",
                  test_keyboard_commit_and_cancel);
    test_run_case("combobox mouse selection and outside cancel",
                  test_mouse_selection_and_outside_cancel);
    test_run_case("combobox viewport width and drawing",
                  test_viewport_width_and_drawing);
    test_run_case("combobox arrow navigation and parent attributes",
                  test_arrow_navigation_and_parent_attributes);
    test_run_case("combobox popup placement and setters",
                  test_popup_placement_above_and_setters);
}
