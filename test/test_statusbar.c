#include "test_support.h"

static void test_mouse_command(void)
{
    TuiStatusItem items[] = {
        { "Save", TUI_KEY_F2, 95 }
    };
    TuiDesktop desktop;
    TuiStatusBar statusbar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_statusbar_init(&statusbar, items, 1);
    tui_add(&desktop.control, &statusbar.control);

    CHECK(test_mouse_action(&desktop, 2, TEST_HEIGHT - 1,
                            TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 95);
    CHECK(event.source == &statusbar.control);
}

void test_statusbar_suite(void)
{
    test_run_case("statusbar mouse command", test_mouse_command);
}
