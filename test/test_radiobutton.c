#include "test_support.h"

static void test_group_selection_and_commands(void)
{
    TuiDesktop desktop;
    TuiRadioButton basic;
    TuiRadioButton forth;
    TuiRadioButton c;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_radiobutton_init(&basic, 1, 1, 12, "BASIC", 1);
    tui_radiobutton_init(&forth, 1, 2, 12, "Forth", 1);
    tui_radiobutton_init(&c, 1, 3, 12, "C", 2);
    tui_add(&desktop.control, &basic.control);
    tui_add(&desktop.control, &forth.control);
    tui_add(&desktop.control, &c.control);
    tui_radiobutton_set_command(&basic, 71);
    tui_radiobutton_set_command(&forth, 72);
    tui_radiobutton_set_checked(&basic, -1);

    CHECK(tui_radiobutton_get_checked(&basic));
    CHECK(!tui_radiobutton_get_checked(&forth));
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event) == 0);

    tui_desktop_set_focus(&desktop, &forth.control);
    CHECK(test_key_event(&desktop, ' ', &event));
    CHECK(!tui_radiobutton_get_checked(&basic));
    CHECK(tui_radiobutton_get_checked(&forth));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 72);
    CHECK(event.source == &forth.control);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(tui_radiobutton_get_checked(&forth));
    CHECK(event.type == TUI_EV_KEY);

    CHECK(test_mouse_action(&desktop, 4, 3,
                            TUI_MOUSE_DOWN, &event));
    CHECK(tui_radiobutton_get_checked(&forth));
    CHECK(event.type == TUI_EV_MOUSE);

    tui_radiobutton_set_checked(&c, 1);
    CHECK(tui_radiobutton_get_checked(&c));
    CHECK(tui_radiobutton_get_checked(&forth));
    tui_radiobutton_set_checked(&c, 0);
    CHECK(!tui_radiobutton_get_checked(&c));
    CHECK(tui_radiobutton_get_checked(&forth));
}

static void test_group_scope_and_single_item(void)
{
    TuiDesktop desktop;
    TuiWindow first_window;
    TuiWindow second_window;
    TuiRadioButton first;
    TuiRadioButton second;

    test_init_desktop(&desktop);
    tui_window_init(&first_window, 0, 0, 20, 8, "First");
    tui_window_init(&second_window, 25, 0, 20, 8, "Second");
    tui_radiobutton_init(&first, 1, 1, 12, "First", 9);
    tui_radiobutton_init(&second, 1, 1, 12, "Second", 9);
    tui_add(&desktop.control, &first_window.control);
    tui_add(&desktop.control, &second_window.control);
    tui_add(&first_window.control, &first.control);
    tui_add(&second_window.control, &second.control);

    tui_radiobutton_set_checked(&first, 1);
    tui_radiobutton_set_checked(&second, 1);
    CHECK(tui_radiobutton_get_checked(&first));
    CHECK(tui_radiobutton_get_checked(&second));

    tui_radiobutton_set_command(&first, TUI_CMD_NONE);
    tui_desktop_set_focus(&desktop, &first.control);
    CHECK(test_key(&desktop, ' '));
    CHECK(tui_radiobutton_get_checked(&first));

    tui_radiobutton_set_checked(&first, 0);
    CHECK(!tui_radiobutton_get_checked(&first));
    CHECK(tui_radiobutton_get_checked(&second));
}

static void test_drawing_and_narrow_width(void)
{
    TuiDesktop desktop;
    TuiRadioButton radio;

    test_init_desktop(&desktop);
    tui_radiobutton_init(&radio, 2, 2, 12, "Forth", 1);
    tui_add(&desktop.control, &radio.control);
    tui_desktop_set_focus(&desktop, &radio.control);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == '(');
    CHECK(test_cell_chars[2][3] == ' ');
    CHECK(test_cell_chars[2][4] == ')');
    CHECK(test_cell_chars[2][6] == 'F');
    CHECK(test_cell_attrs[2][2] ==
          TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY));

    tui_radiobutton_set_checked(&radio, 1);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][3] == TUI_CH_BULLET);

    tui_radiobutton_init(&radio, 0, 4, 1, "", 0);
    tui_draw(&desktop);
    CHECK(test_cell_chars[4][0] == '(');
}

void test_radiobutton_suite(void)
{
    test_run_case("radiobutton group selection and commands",
                  test_group_selection_and_commands);
    test_run_case("radiobutton group scope",
                  test_group_scope_and_single_item);
    test_run_case("radiobutton drawing and widths",
                  test_drawing_and_narrow_width);
}
