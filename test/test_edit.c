#include <string.h>

#include "test_support.h"

static void test_text_and_editing_keys(void)
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

static void test_mouse_and_tab_focus(void)
{
    char buffer[16];
    TuiDesktop desktop;
    TuiButton button;
    TuiEdit edit;

    buffer[0] = '\0';
    test_init_desktop(&desktop);
    tui_button_init(&button, 1, 1, 8, "Next", 1);
    tui_edit_init(&edit, 1, 3, 8, buffer, 16);
    tui_add(&desktop.control, &button.control);
    tui_add(&desktop.control, &edit.control);
    tui_desktop_set_focus(&desktop, &button.control);

    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(tui_desktop_get_focus(&desktop) == &edit.control);
    CHECK(test_key(&desktop, 'A'));
    CHECK(strcmp(tui_edit_get_text(&edit), "A") == 0);

    tui_desktop_set_focus(&desktop, &button.control);
    CHECK(test_mouse_down(&desktop, 2, 3));
    CHECK(tui_desktop_get_focus(&desktop) == &edit.control);
    CHECK(test_key(&desktop, 'B'));
    CHECK(strcmp(tui_edit_get_text(&edit), "AB") == 0);
}

static void test_capacity_set_text_and_mouse(void)
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

static void test_horizontal_viewport_and_drawing(void)
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

void test_edit_suite(void)
{
    test_run_case("edit text and editing keys", test_text_and_editing_keys);
    test_run_case("edit mouse and tab focus", test_mouse_and_tab_focus);
    test_run_case("edit capacity and mouse",
                  test_capacity_set_text_and_mouse);
    test_run_case("edit horizontal viewport",
                  test_horizontal_viewport_and_drawing);
}
