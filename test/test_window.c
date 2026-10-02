#include "test_support.h"

static void test_mouse_drag_capture(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    TuiWindow workspace;
    TuiWindow left;
    TuiWindow right;
    TuiMenuBar menubar;
    TuiStatusBar statusbar;
    TuiEvent event;
    int x;
    int y;

    test_init_desktop(&desktop);
    tui_window_init(&window, 10, 4, 20, 8, "Drag");
    tui_add(&desktop.control, &window.control);

    CHECK(test_mouse_action(&desktop, 12, 4,
                            TUI_MOUSE_DOWN, &event));
    CHECK(window.dragging);
    CHECK(desktop.capture == &window.control);

    CHECK(test_mouse_action(&desktop, 20, 8,
                            TUI_MOUSE_MOVE, &event));
    CHECK(window.control.x == 18);
    CHECK(window.control.y == 8);

    CHECK(test_mouse_action(&desktop, 20, 8,
                            TUI_MOUSE_UP, &event));
    CHECK(!window.dragging);
    CHECK(desktop.capture == 0);

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, 0, 0);
    tui_statusbar_init(&statusbar, 0, 0);
    tui_window_init(&left, 0, 0, 12, 5, "Left");
    left.control.dock = TUI_DOCK_LEFT;
    tui_window_init(&right, 0, 0, 15, 5, "Right");
    right.control.dock = TUI_DOCK_RIGHT;
    tui_window_init(&workspace, 12, 1, 53, 23, "Workspace");
    workspace.control.dock = TUI_DOCK_FILL;
    tui_window_init(&window, 26, 0, 25, 8, "Button");
    tui_add(&desktop.control, &menubar.control);
    tui_add(&desktop.control, &statusbar.control);
    tui_add(&desktop.control, &left.control);
    tui_add(&desktop.control, &right.control);
    tui_add(&desktop.control, &workspace.control);
    tui_add(&workspace.control, &window.control);

    CHECK(test_mouse_action(&desktop, 41, 2,
                            TUI_MOUSE_DOWN, &event));
    CHECK(window.dragging);
    CHECK(desktop.capture == &window.control);

    CHECK(test_mouse_action(&desktop, 50, 7,
                            TUI_MOUSE_MOVE, &event));
    CHECK(window.control.x == 35);
    CHECK(window.control.y == 5);

    tui_control_screen_to_local(&window.control,
                                48, 7, &x, &y);
    CHECK(x == 0);
    CHECK(y == 0);

    CHECK(test_mouse_action(&desktop, 50, 7,
                            TUI_MOUSE_UP, &event));
    CHECK(!window.dragging);
    CHECK(desktop.capture == 0);
}

void test_window_suite(void)
{
    test_run_case("window mouse drag capture", test_mouse_drag_capture);
}
