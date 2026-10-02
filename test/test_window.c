#include <string.h>
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

/* Window at (10,2) 20x5; row 2 is the top border. */
#define WX 10
#define WY 2
#define WW 20
#define WH 5

static void setup_window(TuiDesktop *desktop, TuiWindow *window,
                         const char *title, unsigned flags)
{
    test_init_desktop(desktop);
    tui_window_init(window, WX, WY, WW, WH, title);
    tui_window_set_flags(window, flags);
    tui_add(&desktop->control, &window->control);
    tui_draw(desktop);
}

static int top(int x)
{
    return test_cell_chars[WY][WX + x];
}

static void test_flags_api(void)
{
    TuiWindow window;

    memset(&window, 0xAA, sizeof(window));
    tui_window_init(&window, 0, 0, 10, 5, "W");
    CHECK(tui_window_get_flags(&window) == 0);

    tui_window_set_flags(&window, TUI_WINDOW_FIXED);
    CHECK(tui_window_get_flags(&window) == TUI_WINDOW_FIXED);
    tui_window_add_flags(&window, TUI_WINDOW_ACTIVE_DOUBLE);
    CHECK(tui_window_get_flags(&window) ==
          (TUI_WINDOW_FIXED | TUI_WINDOW_ACTIVE_DOUBLE));
    tui_window_remove_flags(&window, TUI_WINDOW_FIXED);
    CHECK(tui_window_get_flags(&window) == TUI_WINDOW_ACTIVE_DOUBLE);
}

static void test_fixed_window(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    TuiEdit edit;
    char buffer[8];
    TuiEvent event;

    /* Normal window is movable. */
    setup_window(&desktop, &window, "Win", 0);
    CHECK(test_mouse_action(&desktop, WX + 2, WY,
                            TUI_MOUSE_DOWN, &event));
    CHECK(window.dragging);
    CHECK(test_mouse_action(&desktop, WX + 2, WY,
                            TUI_MOUSE_UP, &event));

    /* Fixed window does not start dragging. */
    tui_window_set_flags(&window, TUI_WINDOW_FIXED);
    CHECK(!test_mouse_action(&desktop, WX + 2, WY,
                             TUI_MOUSE_DOWN, &event));
    CHECK(!window.dragging);
    CHECK(desktop.capture == 0);

    /* Programmatic movement is still allowed. */
    window.control.x = 20;
    CHECK(window.control.x == 20);
    window.control.x = WX;

    /* Children still receive focus and events. */
    buffer[0] = '\0';
    tui_edit_init(&edit, 1, 1, 6, buffer, 8);
    tui_add(&window.control, &edit.control);
    CHECK(test_mouse_action(&desktop, WX + 2, WY + 2,
                            TUI_MOUSE_DOWN, &event));
    CHECK(desktop.focused == &edit.control);
    CHECK(test_key(&desktop, 'a'));
    CHECK(buffer[0] == 'a');

    /* Fixed window can be activated by clicking its border. */
    desktop.focused = 0;
    CHECK(!test_mouse_action(&desktop, WX + 2, WY,
                             TUI_MOUSE_DOWN, &event));
    CHECK(desktop.capture == 0);
}

static void test_fixed_during_drag(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    TuiEvent event;

    setup_window(&desktop, &window, "Win", 0);
    CHECK(test_mouse_action(&desktop, WX + 2, WY,
                            TUI_MOUSE_DOWN, &event));
    CHECK(window.dragging);
    CHECK(desktop.capture == &window.control);

    tui_window_add_flags(&window, TUI_WINDOW_FIXED);
    CHECK(!window.dragging);
    CHECK(desktop.capture == 0);

    /* Flag set directly mid-drag is also cancelled on next event. */
    tui_window_remove_flags(&window, TUI_WINDOW_FIXED);
    CHECK(test_mouse_action(&desktop, WX + 2, WY,
                            TUI_MOUSE_DOWN, &event));
    window.flags |= TUI_WINDOW_FIXED;
    CHECK(test_mouse_action(&desktop, WX + 6, WY + 3,
                            TUI_MOUSE_MOVE, &event));
    CHECK(!window.dragging);
    CHECK(desktop.capture == 0);
    CHECK(window.control.x == WX);
}

static void test_title_alignment(void)
{
    TuiDesktop desktop;
    TuiWindow window;

    /* Default (flags == 0) stays centered, single border. */
    setup_window(&desktop, &window, "Win", 0);
    CHECK(top(0) == TUI_CH_TL);
    CHECK(top(7) == ' ' && top(8) == 'W' && top(10) == 'n');
    CHECK(top(11) == ' ' && top(12) == TUI_CH_HLINE);
    CHECK(top(WW - 1) == TUI_CH_TR);

    setup_window(&desktop, &window, "Win", TUI_WINDOW_TITLE_CENTER);
    CHECK(top(8) == 'W');

    setup_window(&desktop, &window, "Win", TUI_WINDOW_TITLE_LEFT);
    CHECK(top(1) == TUI_CH_HLINE);
    CHECK(top(2) == ' ' && top(3) == 'W' && top(5) == 'n');
    CHECK(top(6) == ' ' && top(7) == TUI_CH_HLINE);

    setup_window(&desktop, &window, "Win", TUI_WINDOW_TITLE_RIGHT);
    CHECK(top(13) == ' ' && top(14) == 'W' && top(16) == 'n');
    CHECK(top(17) == ' ' && top(18) == TUI_CH_HLINE);
    CHECK(top(19) == TUI_CH_TR);

    /* Unknown alignment bits fall back to center. */
    setup_window(&desktop, &window, "Win", TUI_WINDOW_TITLE_MASK);
    CHECK(top(8) == 'W');
}

static void test_active_double(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    TuiWindow inner;
    TuiEdit edit;
    char buffer[8];

    buffer[0] = '\0';

    /* Inactive: single border. */
    setup_window(&desktop, &window, "Win",
                 TUI_WINDOW_ACTIVE_DOUBLE);
    CHECK(top(0) == TUI_CH_TL);
    CHECK(top(1) == TUI_CH_HLINE);

    /* Active through a descendant: double border, same cells. */
    tui_window_init(&inner, 1, 1, 10, 3, "In");
    tui_edit_init(&edit, 1, 1, 6, buffer, 8);
    tui_add(&window.control, &inner.control);
    tui_add(&inner.control, &edit.control);
    tui_desktop_set_focus(&desktop, &edit.control);
    tui_draw(&desktop);
    CHECK(top(0) == TUI_CH_DTL);
    CHECK(top(1) == TUI_CH_DHLINE);
    CHECK(top(WW - 1) == TUI_CH_DTR);
    CHECK(test_cell_chars[WY][WX + 7] == ' ');
    CHECK(test_cell_chars[WY + 1][WX] == TUI_CH_DVLINE);
    CHECK(test_cell_chars[WY + WH - 1][WX] == TUI_CH_DBL);
    CHECK(test_cell_chars[WY + WH - 1][WX + WW - 1] == TUI_CH_DBR);
    CHECK(test_cell_chars[WY + WH - 1][WX + 5] == TUI_CH_DHLINE);
    CHECK(test_cell_chars[WY + 1][WX + WW - 1] == TUI_CH_DVLINE);

    /* The inner window has no ACTIVE_DOUBLE: stays single. */
    CHECK(test_cell_chars[WY + 1][WX + 1] == ' ' ||
          test_cell_chars[WY + 1][WX + 1] == TUI_CH_TL);

    /* Focus elsewhere: single again. */
    tui_desktop_set_focus(&desktop, 0);
    tui_draw(&desktop);
    CHECK(top(0) == TUI_CH_TL);

    /* Active without the flag: single. */
    tui_window_set_flags(&window, 0);
    tui_desktop_set_focus(&desktop, &edit.control);
    tui_draw(&desktop);
    CHECK(top(0) == TUI_CH_TL);
    CHECK(top(1) == TUI_CH_HLINE);
}

static void test_active_double_other_window(void)
{
    TuiDesktop desktop;
    TuiWindow a;
    TuiWindow b;

    test_init_desktop(&desktop);
    tui_window_init(&a, 0, 0, 10, 4, "A");
    tui_window_init(&b, 20, 0, 10, 4, "B");
    tui_window_set_flags(&a, TUI_WINDOW_ACTIVE_DOUBLE);
    tui_window_set_flags(&b, TUI_WINDOW_ACTIVE_DOUBLE);
    tui_add(&desktop.control, &a.control);
    tui_add(&desktop.control, &b.control);

    tui_desktop_set_focus(&desktop, &b.control);
    tui_draw(&desktop);
    CHECK(test_cell_chars[0][0] == TUI_CH_TL);
    CHECK(test_cell_chars[0][20] == TUI_CH_DTL);
}

static void test_double_title_alignment(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    unsigned base;

    base = TUI_WINDOW_ACTIVE_DOUBLE;

    setup_window(&desktop, &window, "Win", base | TUI_WINDOW_TITLE_LEFT);
    tui_desktop_set_focus(&desktop, &window.control);
    tui_draw(&desktop);
    CHECK(top(0) == TUI_CH_DTL && top(1) == TUI_CH_DHLINE);
    CHECK(top(3) == 'W' && top(7) == TUI_CH_DHLINE);

    tui_window_set_flags(&window, base | TUI_WINDOW_TITLE_CENTER);
    tui_draw(&desktop);
    CHECK(top(8) == 'W' && top(12) == TUI_CH_DHLINE);
    CHECK(top(6) == TUI_CH_DHLINE);

    tui_window_set_flags(&window, base | TUI_WINDOW_TITLE_RIGHT);
    tui_draw(&desktop);
    CHECK(top(14) == 'W' && top(18) == TUI_CH_DHLINE);
    CHECK(top(19) == TUI_CH_DTR);

    /* Combination with FIXED changes nothing visually. */
    tui_window_set_flags(&window, base | TUI_WINDOW_FIXED |
                                  TUI_WINDOW_TITLE_RIGHT);
    tui_draw(&desktop);
    CHECK(top(14) == 'W' && top(0) == TUI_CH_DTL);
}

static void test_no_title(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    int x;

    setup_window(&desktop, &window, 0, TUI_WINDOW_TITLE_RIGHT);
    for (x = 1; x < WW - 1; ++x)
        CHECK(top(x) == TUI_CH_HLINE);

    setup_window(&desktop, &window, "", TUI_WINDOW_TITLE_LEFT);
    for (x = 1; x < WW - 1; ++x)
        CHECK(top(x) == TUI_CH_HLINE);

    setup_window(&desktop, &window, "", TUI_WINDOW_ACTIVE_DOUBLE);
    tui_desktop_set_focus(&desktop, &window.control);
    tui_draw(&desktop);
    for (x = 1; x < WW - 1; ++x)
        CHECK(top(x) == TUI_CH_DHLINE);
}

static void test_long_and_narrow_titles(void)
{
    static const unsigned aligns[] = {
        TUI_WINDOW_TITLE_LEFT,
        TUI_WINDOW_TITLE_CENTER,
        TUI_WINDOW_TITLE_RIGHT
    };
    TuiDesktop desktop;
    TuiWindow window;
    int i;
    int w;
    int x;

    /* Long title is truncated to width - 4 and keeps the corners. */
    for (i = 0; i < 3; ++i) {
        setup_window(&desktop, &window,
                     "A very long window title", aligns[i]);
        CHECK(top(0) == TUI_CH_TL);
        CHECK(top(WW - 1) == TUI_CH_TR);
        CHECK(top(1) == ' ');
        CHECK(top(2) == 'A');
        CHECK(top(WW - 3) == 'd');
        CHECK(top(WW - 2) == ' ');
        CHECK(test_cell_chars[WY][WX + WW] == test_cell_chars[WY][70]);
        CHECK(test_cell_chars[WY][WX - 1] == test_cell_chars[WY][70]);
    }

    /* Tiny windows never draw outside nor over corners. */
    for (i = 0; i < 3; ++i) {
        for (w = 0; w <= 6; ++w) {
            test_init_desktop(&desktop);
            tui_window_init(&window, WX, WY, w, 3, "Title");
            tui_window_set_flags(&window, aligns[i] |
                                 TUI_WINDOW_ACTIVE_DOUBLE);
            tui_add(&desktop.control, &window.control);
            tui_desktop_set_focus(&desktop, &window.control);
            tui_draw(&desktop);

            for (x = 0; x < TEST_WIDTH; ++x) {
                if (x < WX || x >= WX + w)
                    CHECK(test_cell_chars[WY][x] == test_cell_chars[WY][70]);
            }

            if (w >= 2) {
                CHECK(top(0) == TUI_CH_DTL);
                CHECK(top(w - 1) == TUI_CH_DTR);
            }

            if (w < 5 && w >= 3)
                CHECK(top(1) == TUI_CH_DHLINE);
        }
    }
}
static void test_click_surface_activates_window(void)
{
    TuiDesktop desktop;
    TuiWindow a;
    TuiWindow b;
    TuiWindow empty;
    TuiEdit edit_a;
    TuiEdit edit_b;
    char buf_a[8];
    char buf_b[8];
    TuiEvent event;

    buf_a[0] = '\0';
    buf_b[0] = '\0';
    test_init_desktop(&desktop);
    tui_window_init(&a, 0, 0, 20, 6, "A");
    tui_window_init(&b, 30, 0, 20, 6, "B");
    tui_window_init(&empty, 0, 10, 20, 6, "E");
    tui_edit_init(&edit_a, 1, 1, 6, buf_a, 8);
    tui_edit_init(&edit_b, 1, 1, 6, buf_b, 8);
    tui_add(&desktop.control, &a.control);
    tui_add(&desktop.control, &b.control);
    tui_add(&desktop.control, &empty.control);
    tui_add(&a.control, &edit_a.control);
    tui_add(&b.control, &edit_b.control);
    tui_desktop_set_focus(&desktop, &edit_a.control);

    /* Empty surface of B moves focus to B's edit. */
    test_mouse_action(&desktop, 40, 4, TUI_MOUSE_DOWN, &event);
    CHECK(desktop.focused == &edit_b.control);
    test_mouse_action(&desktop, 40, 4, TUI_MOUSE_UP, &event);

    /* Clicking the active window's surface keeps its focus. */
    test_mouse_action(&desktop, 45, 4, TUI_MOUSE_DOWN, &event);
    CHECK(desktop.focused == &edit_b.control);
    test_mouse_action(&desktop, 45, 4, TUI_MOUSE_UP, &event);

    /* Window without focusable children takes the focus itself. */
    test_mouse_action(&desktop, 5, 13, TUI_MOUSE_DOWN, &event);
    CHECK(desktop.focused == &empty.control);
}

void test_window_suite(void)
{
    test_run_case("window mouse drag capture", test_mouse_drag_capture);
    test_run_case("window flags api", test_flags_api);
    test_run_case("window fixed", test_fixed_window);
    test_run_case("window fixed during drag", test_fixed_during_drag);
    test_run_case("window title alignment", test_title_alignment);
    test_run_case("window active double", test_active_double);
    test_run_case("window active double per window",
                  test_active_double_other_window);
    test_run_case("window double title alignment",
                  test_double_title_alignment);
    test_run_case("window without title", test_no_title);
    test_run_case("window long and narrow titles",
                  test_long_and_narrow_titles);
    test_run_case("click on window surface activates it",
                  test_click_surface_activates_window);
}
