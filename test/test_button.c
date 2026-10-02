#include "test_support.h"

static void test_mouse_capture_and_activation(void)
{
    TuiDesktop desktop;
    TuiButton button;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_button_init(&button, 2, 2, 10, "Run", 73);
    tui_add(&desktop.control, &button.control);

    CHECK(test_mouse_action(&desktop, 4, 2,
                            TUI_MOUSE_DOWN, &event));
    CHECK(button.pressed);
    CHECK(desktop.capture == &button.control);
    CHECK(desktop.focused == &button.control);

    CHECK(test_mouse_action(&desktop, 4, 2,
                            TUI_MOUSE_UP, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 73);
    CHECK(event.source == &button.control);
    CHECK(!button.pressed);
    CHECK(desktop.capture == 0);

    CHECK(test_mouse_action(&desktop, 4, 2,
                            TUI_MOUSE_DOWN, &event));
    CHECK(test_mouse_action(&desktop, 20, 10,
                            TUI_MOUSE_UP, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(!button.pressed);
    CHECK(desktop.capture == 0);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 73);
}

static void test_draw_focus_states(void)
{
    TuiDesktop desktop;
    TuiButton button;

    test_init_desktop(&desktop);
    tui_button_init(&button, 2, 2, 10, "Run", 73);
    tui_add(&desktop.control, &button.control);

    tui_draw(&desktop);
    CHECK(test_cell_chars[2][4] == '[');
    CHECK(test_cell_chars[2][5] == 'R');
    CHECK(test_cell_attrs[2][5] ==
          TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY));

    tui_desktop_set_focus(&desktop, &button.control);
    tui_draw(&desktop);
    CHECK(test_cell_attrs[2][5] ==
          TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY));

    tui_button_init(&button, 2, 2, 2, "Run", 73);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == '[');
    CHECK(test_cell_chars[2][3] == 'R');
}

void test_button_suite(void)
{
    test_run_case("button mouse capture and activation",
                  test_mouse_capture_and_activation);
    test_run_case("button drawing focus states", test_draw_focus_states);
}
