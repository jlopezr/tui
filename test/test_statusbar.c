#include "test_support.h"

static void test_mouse_command(void)
{
    static TuiStatusItem items[] = {
        { "Save", TUI_KEY_F2, 95 },
        { "About", TUI_KEY_NONE, 96 }
    };
    TuiDesktop desktop;
    TuiStatusBar statusbar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_statusbar_init(&statusbar, items, 2);
    tui_add(&desktop.control, &statusbar.control);
    tui_statusbar_set_text(&statusbar, "Saved");

    CHECK(test_mouse_action(&desktop, 2, TEST_HEIGHT - 1,
                            TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 95);
    CHECK(event.source == &statusbar.control);

    CHECK(test_mouse_action(&desktop, 13, TEST_HEIGHT - 1,
                            TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 96);

    CHECK(!test_mouse_action(&desktop, 16, TEST_HEIGHT - 1,
                             TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(!test_mouse_action(&desktop, 2, TEST_HEIGHT - 2,
                             TUI_MOUSE_DOWN, &event));

    tui_draw(&desktop);
    CHECK(test_cell_chars[TEST_HEIGHT - 1][1] == 'F');
    CHECK(test_cell_chars[TEST_HEIGHT - 1][4] == 'S');
    CHECK(test_cell_chars[TEST_HEIGHT - 1][10] == 'A');
    CHECK(test_cell_chars[TEST_HEIGHT - 1][74] == 'S');
    CHECK(test_cell_chars[TEST_HEIGHT - 1][78] == 'd');

    tui_statusbar_set_text(&statusbar, 0);
    tui_draw(&desktop);
    CHECK(test_cell_chars[TEST_HEIGHT - 1][78] == ' ');
}

void test_statusbar_suite(void)
{
    test_run_case("statusbar mouse command", test_mouse_command);
}
