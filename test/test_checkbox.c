#include "test_support.h"

static void test_state_and_commands(void)
{
    TuiDesktop desktop;
    TuiCheckBox checkbox;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_checkbox_init(&checkbox, 2, 2, 16, "Mouse");
    tui_add(&desktop.control, &checkbox.control);
    tui_desktop_set_focus(&desktop, &checkbox.control);
    tui_checkbox_set_command(&checkbox, 61);

    CHECK(!tui_checkbox_get_checked(&checkbox));
    CHECK(test_key_event(&desktop, ' ', &event));
    CHECK(tui_checkbox_get_checked(&checkbox));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 61);
    CHECK(event.source == &checkbox.control);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(!tui_checkbox_get_checked(&checkbox));
    CHECK(event.type == TUI_EV_COMMAND);

    CHECK(test_mouse_action(&desktop, 13, 2,
                            TUI_MOUSE_DOWN, &event));
    CHECK(tui_checkbox_get_checked(&checkbox));
    CHECK(event.type == TUI_EV_COMMAND);

    tui_checkbox_set_checked(&checkbox, -2);
    CHECK(tui_checkbox_get_checked(&checkbox) == 1);
    tui_checkbox_set_checked(&checkbox, 0);
    CHECK(!tui_checkbox_get_checked(&checkbox));

    tui_checkbox_set_command(&checkbox, TUI_CMD_NONE);
    CHECK(test_key_event(&desktop, ' ', &event));
    CHECK(tui_checkbox_get_checked(&checkbox));
    CHECK(event.type == TUI_EV_KEY);
}

static void test_drawing_and_small_widths(void)
{
    TuiDesktop desktop;
    TuiCheckBox checkbox;

    test_init_desktop(&desktop);
    tui_checkbox_init(&checkbox, 2, 2, 12, "Mouse");
    tui_add(&desktop.control, &checkbox.control);
    tui_desktop_set_focus(&desktop, &checkbox.control);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == '[');
    CHECK(test_cell_chars[2][3] == ' ');
    CHECK(test_cell_chars[2][4] == ']');
    CHECK(test_cell_chars[2][6] == 'M');
    CHECK(test_cell_attrs[2][2] ==
          TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY));

    tui_checkbox_set_checked(&checkbox, 1);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][3] == 'x');

    tui_checkbox_init(&checkbox, 0, 4, 1, "");
    tui_draw(&desktop);
    CHECK(test_cell_chars[4][0] == '[');

    tui_checkbox_init(&checkbox, 0, 5, 0, 0);
    tui_draw(&desktop);
    CHECK(test_cell_chars[5][0] != '[');
}

void test_checkbox_suite(void)
{
    test_run_case("checkbox state and commands", test_state_and_commands);
    test_run_case("checkbox drawing and widths",
                  test_drawing_and_small_widths);
}
