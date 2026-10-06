#include "test_support.h"

static void test_control_tree_management(void)
{
    TuiDesktop desktop;
    TuiLabel first;
    TuiLabel second;
    TuiLabel third;

    test_init_desktop(&desktop);
    tui_label_init(&first, 0, 0, "first");
    tui_label_init(&second, 0, 1, "second");
    tui_label_init(&third, 0, 2, "third");

    tui_add(&desktop.control, &first.control);
    tui_add(&desktop.control, &second.control);
    tui_add(&desktop.control, &third.control);
    CHECK(desktop.control.first == &first.control);
    CHECK(desktop.control.last == &third.control);
    CHECK(first.control.next == &second.control);
    CHECK(second.control.prev == &first.control);
    CHECK(second.control.next == &third.control);
    CHECK(third.control.prev == &second.control);

    tui_add(&desktop.control, &second.control);
    CHECK(desktop.control.last == &third.control);
    CHECK(second.control.prev == &first.control);
    CHECK(second.control.next == &third.control);

    tui_bring_to_front(&first.control);
    CHECK(desktop.control.first == &second.control);
    CHECK(desktop.control.last == &first.control);
    CHECK(third.control.next == &first.control);
    CHECK(first.control.prev == &third.control);

    tui_bring_to_front(&first.control);
    tui_bring_to_front(&second.control);
    CHECK(desktop.control.first == &third.control);
    CHECK(desktop.control.last == &second.control);

    tui_remove(&third.control);
    CHECK(third.control.parent == 0);
    CHECK(desktop.control.first == &first.control);
    CHECK(first.control.prev == 0);
    CHECK(first.control.next == &second.control);

    tui_remove(&first.control);
    CHECK(desktop.control.first == &second.control);
    CHECK(desktop.control.last == &second.control);
    CHECK(second.control.prev == 0);
    CHECK(second.control.next == 0);

    tui_remove(&second.control);
    CHECK(desktop.control.first == 0);
    CHECK(desktop.control.last == 0);
    tui_remove(&second.control);
}

static void test_docking_and_hit_testing(void)
{
    TuiDesktop desktop;
    TuiMenuBar menubar;
    TuiStatusBar statusbar;
    TuiWindow left;
    TuiWindow right;
    TuiWindow workspace;
    TuiWindow panel;
    TuiWindow panel_right;
    TuiLabel first;
    TuiLabel second;
    TuiLabel hidden;

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, 0, 0);
    tui_statusbar_init(&statusbar, 0, 0);
    menubar.control.height = 1;
    statusbar.control.height = 1;
    tui_window_init(&left, 0, 0, 10, 1, "Left");
    left.control.dock = TUI_DOCK_LEFT;
    tui_window_init(&right, 0, 0, 15, 1, "Right");
    right.control.dock = TUI_DOCK_RIGHT;
    tui_window_init(&workspace, 0, 0, 1, 1, "Workspace");
    workspace.control.dock = TUI_DOCK_FILL;
    tui_window_init(&panel, 2, 2, 20, 10, "Panel");
    tui_window_init(&panel_right, 0, 0, 5, 1, "Right");
    panel_right.control.dock = TUI_DOCK_RIGHT;
    tui_label_init(&first, 2, 2, "first");
    tui_label_init(&second, 2, 2, "second");
    tui_label_init(&hidden, 0, 0, "hidden");
    hidden.control.flags &= ~TUI_VISIBLE;
    hidden.control.dock = TUI_DOCK_LEFT;

    tui_add(&desktop.control, &menubar.control);
    tui_add(&desktop.control, &statusbar.control);
    tui_add(&desktop.control, &left.control);
    tui_add(&desktop.control, &right.control);
    tui_add(&desktop.control, &workspace.control);
    tui_add(&workspace.control, &hidden.control);
    tui_add(&workspace.control, &panel.control);
    tui_add(&panel.control, &panel_right.control);
    tui_add(&panel.control, &first.control);
    tui_add(&panel.control, &second.control);

    tui_draw(&desktop);
    CHECK(menubar.control.x == 0);
    CHECK(menubar.control.y == 0);
    CHECK(menubar.control.width == TEST_WIDTH);
    CHECK(statusbar.control.y == TEST_HEIGHT - 1);
    CHECK(statusbar.control.width == TEST_WIDTH);
    CHECK(left.control.x == 0);
    CHECK(left.control.y == 1);
    CHECK(left.control.height == TEST_HEIGHT - 2);
    CHECK(right.control.x == TEST_WIDTH - 15);
    CHECK(right.control.height == TEST_HEIGHT - 2);
    CHECK(workspace.control.x == 10);
    CHECK(workspace.control.y == 1);
    CHECK(workspace.control.width == TEST_WIDTH - 25);
    CHECK(workspace.control.height == TEST_HEIGHT - 2);
    CHECK(panel.control.x == 2);
    CHECK(panel_right.control.x == 13);
    CHECK(panel_right.control.y == 0);
    CHECK(panel_right.control.width == 5);
    CHECK(panel_right.control.height == 8);
    CHECK(hidden.control.x == 0);
    CHECK(test_cell_chars[6][31] == TUI_CH_VLINE);
    CHECK(test_cell_chars[12][29] == TUI_CH_HLINE);
    CHECK(test_cell_chars[12][31] == TUI_CH_BR);

    CHECK(tui_hit_test(&desktop.control, 16, 7) ==
          &second.control);
    CHECK(tui_hit_test(&desktop.control, 16, 7) !=
          &first.control);
    CHECK(tui_hit_test(&desktop.control, 14, 5) ==
          &panel.control);
    CHECK(tui_hit_test(&desktop.control, 1, 1) ==
          &left.control);
    CHECK(tui_hit_test(&desktop.control, 79, 23) ==
          &right.control);
    CHECK(tui_hit_test(&desktop.control, 10, 1) ==
          &workspace.control);
    CHECK(tui_hit_test(&desktop.control, TEST_WIDTH, 10) == 0);
}

static void test_focus_navigation_and_capture(void)
{
    TuiDesktop desktop;
    TuiButton first;
    TuiButton disabled;
    TuiEdit edit;
    TuiButton hidden;
    TuiEvent event;
    char buffer[16];

    buffer[0] = '\0';
    test_init_desktop(&desktop);
    tui_button_init(&first, 0, 0, 8, "First", 51);
    tui_button_init(&disabled, 0, 1, 8, "Skip", 52);
    disabled.control.flags &= ~TUI_ENABLED;
    tui_edit_init(&edit, 0, 2, 8, buffer, 16);
    tui_button_init(&hidden, 0, 3, 8, "Hidden", 53);
    hidden.control.flags &= ~TUI_VISIBLE;

    tui_add(&desktop.control, &first.control);
    tui_add(&desktop.control, &disabled.control);
    tui_add(&desktop.control, &edit.control);
    tui_add(&desktop.control, &hidden.control);

    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(tui_desktop_get_focus(&desktop) == &first.control);
    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(tui_desktop_get_focus(&desktop) == &edit.control);
    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(tui_desktop_get_focus(&desktop) == &first.control);

    tui_desktop_set_focus(&desktop, 0);
    CHECK(test_key(&desktop, TUI_KEY_TAB));
    CHECK(tui_desktop_get_focus(&desktop) == &first.control);

    tui_desktop_set_focus(&desktop, &edit.control);
    tui_desktop_set_capture(&desktop, &first.control);
    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 51);
    CHECK(event.source == &first.control);
    CHECK(tui_desktop_get_focus(&desktop) == &edit.control);

    tui_desktop_clear_capture(&desktop);
    CHECK(desktop.capture == 0);
    CHECK(tui_desktop_get_focus(&desktop) == &edit.control);
    CHECK(tui_desktop_get_focus(&desktop) != &hidden.control);
}

/* Reads one event and says whether it is a mouse event at (x, y) with this action. */
static int read_mouse(int x, int y, int action)
{
    TuiEvent event;

    tui_read_event(&event);

    return event.type == TUI_EV_MOUSE &&
           event.mouse_x == x &&
           event.mouse_y == y &&
           event.mouse_action == action;
}

static int read_key(int key)
{
    TuiEvent event;

    tui_read_event(&event);

    return event.type == TUI_EV_KEY && event.key == key;
}

/* Mouse moves one after another come out as one, at the last place. */
static void test_read_event_merges_mouse_moves(void)
{
    test_console_clear_input();
    test_console_push_mouse(1, 1, TUI_MOUSE_MOVE, 0);
    test_console_push_mouse(2, 1, TUI_MOUSE_MOVE, 0);
    test_console_push_mouse(3, 2, TUI_MOUSE_MOVE, 0);
    test_console_push_mouse(4, 2, TUI_MOUSE_MOVE, TUI_MOUSE_LEFT);

    CHECK(read_mouse(4, 2, TUI_MOUSE_MOVE));
    CHECK(test_console_input_left() == 0);
}

/*
 * What is not a move is never merged and never loses its place: the move before
 * it is delivered first, then the press, and the keys and releases after it.
 */
static void test_read_event_keeps_everything_else_in_order(void)
{
    test_console_clear_input();
    test_console_push_mouse(1, 1, TUI_MOUSE_MOVE, 0);
    test_console_push_mouse(2, 1, TUI_MOUSE_MOVE, 0);
    test_console_push_mouse(2, 1, TUI_MOUSE_DOWN, TUI_MOUSE_LEFT);
    test_console_push_mouse(3, 1, TUI_MOUSE_MOVE, TUI_MOUSE_LEFT);
    test_console_push_mouse(4, 1, TUI_MOUSE_MOVE, TUI_MOUSE_LEFT);
    test_console_push_key('a');
    test_console_push_mouse(4, 1, TUI_MOUSE_MOVE, 0);
    test_console_push_mouse(5, 1, TUI_MOUSE_UP, TUI_MOUSE_LEFT);
    test_console_push_mouse(6, 1, TUI_MOUSE_UP, TUI_MOUSE_LEFT);

    CHECK(read_mouse(2, 1, TUI_MOUSE_MOVE));
    CHECK(read_mouse(2, 1, TUI_MOUSE_DOWN));
    CHECK(read_mouse(4, 1, TUI_MOUSE_MOVE));
    CHECK(read_key('a'));
    CHECK(read_mouse(4, 1, TUI_MOUSE_MOVE));
    CHECK(read_mouse(5, 1, TUI_MOUSE_UP));
    CHECK(read_mouse(6, 1, TUI_MOUSE_UP));
    CHECK(test_console_input_left() == 0);
}

/* Keys alone, and a lone move, pass straight through. */
static void test_read_event_passes_single_events(void)
{
    test_console_clear_input();
    test_console_push_key('x');
    test_console_push_key('x');
    test_console_push_key(TUI_KEY_ENTER);
    test_console_push_mouse(7, 3, TUI_MOUSE_MOVE, 0);
    test_console_push_key('y');

    CHECK(read_key('x'));
    CHECK(read_key('x'));
    CHECK(read_key(TUI_KEY_ENTER));
    CHECK(read_mouse(7, 3, TUI_MOUSE_MOVE));
    CHECK(read_key('y'));

    /* The read-ahead event was a key: nothing is left over from before. */
    test_console_push_mouse(1, 1, TUI_MOUSE_MOVE, 0);
    CHECK(read_mouse(1, 1, TUI_MOUSE_MOVE));
}

void test_core_suite(void)
{
    test_run_case("core control tree management",
                  test_control_tree_management);
    test_run_case("core docking and hit testing",
                  test_docking_and_hit_testing);
    test_run_case("core focus navigation and capture",
                  test_focus_navigation_and_capture);
    test_run_case("read event merges mouse moves",
                  test_read_event_merges_mouse_moves);
    test_run_case("read event keeps everything else in order",
                  test_read_event_keeps_everything_else_in_order);
    test_run_case("read event passes single events",
                  test_read_event_passes_single_events);
}
