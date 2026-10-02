#include "test_support.h"

#define SB_CMD 91

static void setup(TuiDesktop *desktop, TuiScrollBar *sb, int orientation)
{
    test_init_desktop(desktop);
    tui_scrollbar_init(sb, 2, 2, 12, orientation, SB_CMD);
    tui_add(&desktop->control, &sb->control);
    tui_scrollbar_set_range(sb, 0, 100);
    tui_scrollbar_set_page(sb, 20);
}

static void test_normalization(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;

    setup(&desktop, &sb, TUI_VERTICAL);

    CHECK(sb.control.width == 1 && sb.control.height == 12);
    tui_scrollbar_set_value(&sb, 500);
    CHECK(tui_scrollbar_get_value(&sb) == 80);
    tui_scrollbar_set_value(&sb, -5);
    CHECK(tui_scrollbar_get_value(&sb) == 0);

    tui_scrollbar_set_value(&sb, 70);
    tui_scrollbar_set_page(&sb, 50);
    CHECK(tui_scrollbar_get_value(&sb) == 50);
    tui_scrollbar_set_range(&sb, 0, 30);
    CHECK(tui_scrollbar_get_value(&sb) == 0);
    tui_scrollbar_set_page(&sb, -3);
    CHECK(sb.page == 0);
    tui_scrollbar_set_range(&sb, 10, 5);
    CHECK(sb.max == 10 && tui_scrollbar_get_value(&sb) == 10);

    tui_scrollbar_init(&sb, 0, 0, 5, TUI_HORIZONTAL, 0);
    CHECK(sb.control.width == 5 && sb.control.height == 1);
}

static void test_setters_do_not_emit(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;

    setup(&desktop, &sb, TUI_VERTICAL);
    tui_scrollbar_set_value(&sb, 30);
    CHECK(test_key_event(&desktop, 'x', &event) == 0);
    CHECK(tui_scrollbar_get_value(&sb) == 30);
}

static void test_draw_vertical(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;

    setup(&desktop, &sb, TUI_VERTICAL);
    tui_scrollbar_set_value(&sb, 40);
    tui_draw(&desktop);

    CHECK(test_cell_chars[2][2] == TUI_CH_UP_TRIANGLE);
    CHECK(test_cell_chars[13][2] == TUI_CH_DOWN_TRIANGLE);
    CHECK(test_cell_chars[3][2] == TUI_CH_SCROLL_TRACK);
    CHECK(test_cell_chars[6][2] == TUI_CH_SCROLL_TRACK);
    CHECK(test_cell_chars[7][2] == TUI_CH_SCROLL_THUMB);
    CHECK(test_cell_chars[8][2] == TUI_CH_SCROLL_THUMB);
    CHECK(test_cell_chars[9][2] == TUI_CH_SCROLL_TRACK);

    tui_desktop_set_focus(&desktop, &sb.control);
    tui_draw(&desktop);
    CHECK(test_cell_attrs[7][2] == TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY));
    CHECK(test_cell_attrs[3][2] == TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY));
}

static void test_draw_horizontal_and_resize(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;

    setup(&desktop, &sb, TUI_HORIZONTAL);
    tui_scrollbar_set_value(&sb, 80);
    tui_draw(&desktop);

    CHECK(test_cell_chars[2][2] == TUI_CH_LEFT_TRIANGLE);
    CHECK(test_cell_chars[2][13] == TUI_CH_RIGHT_TRIANGLE);
    CHECK(test_cell_chars[2][12] == TUI_CH_SCROLL_THUMB);
    CHECK(test_cell_chars[2][11] == TUI_CH_SCROLL_THUMB);
    CHECK(test_cell_chars[2][10] == TUI_CH_SCROLL_TRACK);

    test_reset_screen();
    sb.control.width = 7;
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][8] == TUI_CH_RIGHT_TRIANGLE);
}

static void test_degenerate_lengths(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;
    int len;

    for (len = 0; len <= 3; ++len) {
        setup(&desktop, &sb, TUI_VERTICAL);
        sb.control.height = len;
        tui_draw(&desktop);

        if (len > 0)
            CHECK(test_cell_chars[2][2] == TUI_CH_UP_TRIANGLE);

        if (len > 1)
            CHECK(test_cell_chars[2 + len - 1][2] ==
                  TUI_CH_DOWN_TRIANGLE);

        if (len == 3)
            CHECK(test_cell_chars[3][2] == TUI_CH_SCROLL_THUMB);

        if (len > 0)
            CHECK(test_mouse_action(&desktop, 2, 2 + len - 1,
                                    TUI_MOUSE_DOWN, &event));
    }

    /* Without track there is no thumb to drag. */
    setup(&desktop, &sb, TUI_VERTICAL);
    sb.control.height = 2;
    CHECK(test_mouse_action(&desktop, 2, 3, TUI_MOUSE_DOWN, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 1);
}

static void test_keyboard(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;

    setup(&desktop, &sb, TUI_VERTICAL);
    tui_desktop_set_focus(&desktop, &sb.control);

    CHECK(test_key_event(&desktop, TUI_KEY_DOWN, &event));
    CHECK(event.type == TUI_EV_COMMAND && event.command == SB_CMD);
    CHECK(event.source == &sb.control);
    CHECK(tui_scrollbar_get_value(&sb) == 1);

    CHECK(test_key_event(&desktop, TUI_KEY_UP, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 0);

    /* At the limit the key is consumed without a command. */
    CHECK(test_key_event(&desktop, TUI_KEY_UP, &event));
    CHECK(event.type == TUI_EV_KEY);

    CHECK(test_key_event(&desktop, TUI_KEY_PAGEDOWN, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 20);
    CHECK(test_key_event(&desktop, TUI_KEY_PAGEUP, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 0);
    CHECK(test_key_event(&desktop, TUI_KEY_END, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 80);
    CHECK(test_key_event(&desktop, TUI_KEY_HOME, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 0);

    CHECK(test_key_event(&desktop, TUI_KEY_LEFT, &event) == 0);
    CHECK(test_key_event(&desktop, 'x', &event) == 0);

    setup(&desktop, &sb, TUI_HORIZONTAL);
    tui_desktop_set_focus(&desktop, &sb.control);
    CHECK(test_key_event(&desktop, TUI_KEY_RIGHT, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 1);
    CHECK(test_key_event(&desktop, TUI_KEY_LEFT, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 0);
    CHECK(test_key_event(&desktop, TUI_KEY_DOWN, &event) == 0);

    /* No command configured: value changes, no COMMAND. */
    tui_scrollbar_set_command(&sb, TUI_CMD_NONE);
    CHECK(test_key_event(&desktop, TUI_KEY_RIGHT, &event));
    CHECK(event.type == TUI_EV_KEY);
    CHECK(tui_scrollbar_get_value(&sb) == 1);
}

static void test_mouse_arrows_and_track(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;

    setup(&desktop, &sb, TUI_VERTICAL);
    tui_scrollbar_set_value(&sb, 40);

    CHECK(test_mouse_action(&desktop, 2, 13, TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_COMMAND && event.command == SB_CMD);
    CHECK(tui_scrollbar_get_value(&sb) == 41);
    CHECK(desktop.focused == &sb.control);

    CHECK(test_mouse_action(&desktop, 2, 2, TUI_MOUSE_DOWN, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 40);

    CHECK(test_mouse_action(&desktop, 2, 12, TUI_MOUSE_DOWN, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 60);
    CHECK(test_mouse_action(&desktop, 2, 3, TUI_MOUSE_DOWN, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 40);

    tui_scrollbar_set_value(&sb, 0);
    CHECK(test_mouse_action(&desktop, 2, 2, TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(desktop.capture == 0);

    /* Release without a drag in progress is not handled. */
    CHECK(test_mouse_action(&desktop, 2, 2, TUI_MOUSE_UP, &event) == 0);
}

static void test_thumb_drag(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;

    setup(&desktop, &sb, TUI_VERTICAL);

    /* Thumb occupies rows 3-4; grab the second cell. */
    CHECK(test_mouse_action(&desktop, 2, 4, TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(sb.dragging && sb.drag_offset == 1);
    CHECK(desktop.capture == &sb.control);

    CHECK(test_mouse_action(&desktop, 2, 8, TUI_MOUSE_MOVE, &event));
    CHECK(event.type == TUI_EV_COMMAND && event.command == SB_CMD);
    CHECK(tui_scrollbar_get_value(&sb) == 40);

    /* Same position: no change, no command. */
    CHECK(test_mouse_action(&desktop, 2, 8, TUI_MOUSE_MOVE, &event));
    CHECK(event.type == TUI_EV_MOUSE);

    /* Outside the control the thumb clamps to both ends. */
    CHECK(test_mouse_action(&desktop, 30, 40, TUI_MOUSE_MOVE, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 80);
    CHECK(test_mouse_action(&desktop, 30, 0, TUI_MOUSE_MOVE, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 0);

    CHECK(test_mouse_action(&desktop, 2, 8, TUI_MOUSE_UP, &event));
    CHECK(!sb.dragging);
    CHECK(desktop.capture == 0);
}

static void test_horizontal_drag(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;

    setup(&desktop, &sb, TUI_HORIZONTAL);

    CHECK(test_mouse_action(&desktop, 3, 2, TUI_MOUSE_DOWN, &event));
    CHECK(sb.dragging && sb.drag_offset == 0);
    CHECK(test_mouse_action(&desktop, 11, 2, TUI_MOUSE_MOVE, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 80);
    CHECK(test_mouse_action(&desktop, 11, 2, TUI_MOUSE_UP, &event));
    CHECK(!sb.dragging);
}

static void test_full_page_and_disabled(void)
{
    TuiDesktop desktop;
    TuiScrollBar sb;
    TuiEvent event;

    setup(&desktop, &sb, TUI_VERTICAL);
    tui_scrollbar_set_page(&sb, 100);
    tui_draw(&desktop);
    CHECK(test_cell_chars[3][2] == TUI_CH_SCROLL_THUMB);
    CHECK(test_cell_chars[12][2] == TUI_CH_SCROLL_THUMB);

    CHECK(test_mouse_action(&desktop, 2, 5, TUI_MOUSE_DOWN, &event));
    CHECK(sb.dragging);
    CHECK(test_mouse_action(&desktop, 2, 9, TUI_MOUSE_MOVE, &event));
    CHECK(tui_scrollbar_get_value(&sb) == 0);
    CHECK(test_mouse_action(&desktop, 2, 9, TUI_MOUSE_UP, &event));

    tui_scrollbar_set_page(&sb, 20);
    sb.control.flags &= ~TUI_ENABLED;
    tui_desktop_set_focus(&desktop, 0);
    CHECK(test_mouse_action(&desktop, 2, 13, TUI_MOUSE_DOWN, &event));
    CHECK(desktop.focused != &sb.control);
}

void test_scrollbar_suite(void)
{
    test_run_case("scrollbar normalization", test_normalization);
    test_run_case("scrollbar setters do not emit",
                  test_setters_do_not_emit);
    test_run_case("scrollbar vertical drawing", test_draw_vertical);
    test_run_case("scrollbar horizontal drawing and resize",
                  test_draw_horizontal_and_resize);
    test_run_case("scrollbar degenerate lengths",
                  test_degenerate_lengths);
    test_run_case("scrollbar keyboard", test_keyboard);
    test_run_case("scrollbar mouse arrows and track",
                  test_mouse_arrows_and_track);
    test_run_case("scrollbar thumb drag", test_thumb_drag);
    test_run_case("scrollbar horizontal drag", test_horizontal_drag);
    test_run_case("scrollbar full page and disabled",
                  test_full_page_and_disabled);
}
