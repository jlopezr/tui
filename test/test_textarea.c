#include <string.h>

#include "test_support.h"

static void ta_setup(TuiDesktop *desktop, TuiTextArea *area,
                     char *buffer, int capacity,
                     int width, int height, const char *text)
{
    test_init_desktop(desktop);
    buffer[0] = '\0';
    tui_textarea_init(area, 2, 2, width, height, buffer, capacity);
    tui_textarea_set_text(area, text);
    tui_add(&desktop->control, &area->control);
    tui_desktop_set_focus(desktop, &area->control);
}

static int vis(const TuiScrollBar *sb)
{
    return (sb->control.flags & TUI_VISIBLE) != 0;
}

static int same(const TuiTextArea *area, const char *expected)
{
    return strcmp(tui_textarea_get_text(area), expected) == 0;
}

static void test_ta_text(void)
{
    char buf[32];
    char small[1];
    char two[2];
    char pre[16];
    TuiDesktop desktop;
    TuiTextArea area;

    ta_setup(&desktop, &area, buf, 32, 10, 4, "");
    CHECK(same(&area, ""));
    CHECK(area.cursor_pos == 0);
    tui_draw(&desktop);
    CHECK(test_cursor_visible && test_cursor_x == 2 && test_cursor_y == 2);

    strcpy(pre, "Hello\nWorld");
    test_init_desktop(&desktop);
    tui_textarea_init(&area, 2, 2, 10, 4, pre, 16);
    tui_add(&desktop.control, &area.control);
    CHECK(same(&area, "Hello\nWorld"));
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == 'H');
    CHECK(test_cell_chars[3][2] == 'W');
    CHECK(test_cell_chars[3][6] == 'd');

    /* Unterminated initial buffer is cut inside capacity. */
    memset(pre, 'x', sizeof(pre));
    tui_textarea_init(&area, 0, 0, 10, 4, pre, 4);
    CHECK(pre[3] == '\0' && strlen(pre) == 3);

    /* set_text copies and truncates. */
    ta_setup(&desktop, &area, buf, 6, 10, 4, "abcdefghij");
    CHECK(same(&area, "abcde"));
    tui_textarea_set_text(&area, "xy");
    CHECK(same(&area, "xy"));
    tui_textarea_set_text(&area, 0);
    CHECK(same(&area, ""));

    ta_setup(&desktop, &area, small, 1, 10, 4, "abc");
    CHECK(same(&area, ""));
    CHECK(test_key(&desktop, 'a'));
    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(same(&area, ""));

    ta_setup(&desktop, &area, two, 2, 10, 4, "");
    CHECK(test_key(&desktop, 'a'));
    CHECK(test_key(&desktop, 'b'));
    CHECK(same(&area, "a") && two[1] == '\0');
    tui_textarea_set_text(&area, "");
    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(same(&area, "\n"));

    /* Degenerate arguments are safe. */
    tui_textarea_init(&area, 0, 0, 5, 3, 0, 0);
    tui_textarea_set_text(&area, "abc");
    CHECK(same(&area, ""));
    tui_textarea_set_text(0, "abc");
    tui_textarea_set_readonly(0, 1);
    CHECK(strcmp(tui_textarea_get_text(0), "") == 0);
}

static void test_ta_editing(void)
{
    char buf[16];
    TuiDesktop desktop;
    TuiTextArea area;

    ta_setup(&desktop, &area, buf, 16, 10, 4, "bd");
    CHECK(test_key(&desktop, 'a'));
    CHECK(same(&area, "abd"));

    ta_setup(&desktop, &area, buf, 16, 10, 4, "bd");
    test_key(&desktop, TUI_KEY_RIGHT);
    test_key(&desktop, 'c');
    CHECK(same(&area, "bcd"));
    test_key(&desktop, TUI_KEY_END);
    test_key(&desktop, 'e');
    CHECK(same(&area, "bcde"));

    test_key(&desktop, TUI_KEY_LEFT);
    test_key(&desktop, TUI_KEY_ENTER);
    CHECK(same(&area, "bcd\ne"));
    CHECK(area.cursor_pos == 4);

    /* Backspace at line start joins lines. */
    test_key(&desktop, TUI_KEY_BACKSPACE);
    CHECK(same(&area, "bcde"));
    CHECK(area.cursor_pos == 3);

    test_key(&desktop, TUI_KEY_HOME);
    test_key(&desktop, TUI_KEY_BACKSPACE);
    CHECK(same(&area, "bcde"));
    test_key(&desktop, TUI_KEY_DELETE);
    CHECK(same(&area, "cde"));

    /* Delete over a newline joins; delete at EOF does nothing. */
    tui_textarea_set_text(&area, "ab\ncd");
    test_key(&desktop, TUI_KEY_RIGHT);
    test_key(&desktop, TUI_KEY_RIGHT);
    test_key(&desktop, TUI_KEY_DELETE);
    CHECK(same(&area, "abcd"));
    test_key(&desktop, TUI_KEY_END);
    test_key(&desktop, TUI_KEY_DELETE);
    CHECK(same(&area, "abcd"));

    /* Special and non-text keys are not inserted. */
    CHECK(!test_key(&desktop, TUI_KEY_F1));
    CHECK(same(&area, "abcd"));

    /* Full buffer. */
    ta_setup(&desktop, &area, buf, 4, 10, 4, "abc");
    test_key(&desktop, TUI_KEY_END);
    test_key(&desktop, 'x');
    test_key(&desktop, TUI_KEY_ENTER);
    CHECK(same(&area, "abc") && buf[3] == '\0');

    /* Many Enter/Delete round trips. */
    ta_setup(&desktop, &area, buf, 16, 10, 4, "ab");
    test_key(&desktop, TUI_KEY_ENTER);
    test_key(&desktop, TUI_KEY_ENTER);
    test_key(&desktop, TUI_KEY_ENTER);
    CHECK(same(&area, "\n\n\nab"));
    test_key(&desktop, TUI_KEY_BACKSPACE);
    test_key(&desktop, TUI_KEY_BACKSPACE);
    test_key(&desktop, TUI_KEY_DELETE);
    CHECK(same(&area, "\nb"));
    CHECK(area.cursor_pos == 1);
}

static void test_ta_cursor(void)
{
    char buf[64];
    TuiDesktop desktop;
    TuiTextArea area;

    ta_setup(&desktop, &area, buf, 64, 10, 5, "abcdef\nxy\n\nlast\n");
    test_key(&desktop, TUI_KEY_END);
    CHECK(area.cursor_pos == 6);
    test_key(&desktop, TUI_KEY_RIGHT);
    CHECK(area.cursor_pos == 7);
    test_key(&desktop, TUI_KEY_LEFT);
    CHECK(area.cursor_pos == 6);
    test_key(&desktop, TUI_KEY_HOME);
    CHECK(area.cursor_pos == 0);

    /* Column is kept; shorter lines clamp to their end. */
    test_key(&desktop, TUI_KEY_END);
    test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 9);
    test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 10);
    test_key(&desktop, TUI_KEY_UP);
    CHECK(area.cursor_pos == 7);
    test_key(&desktop, TUI_KEY_UP);
    CHECK(area.cursor_pos == 0);

    /* Last empty line from the trailing newline. */
    tui_textarea_set_text(&area, "abc\n");
    test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 4);
    test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 4);
    test_key(&desktop, TUI_KEY_END);
    CHECK(area.cursor_pos == 4);
    test_key(&desktop, TUI_KEY_RIGHT);
    CHECK(area.cursor_pos == 4);
    test_key(&desktop, TUI_KEY_UP);
    CHECK(area.cursor_pos == 0);
    test_key(&desktop, TUI_KEY_UP);
    CHECK(area.cursor_pos == 0);
    test_key(&desktop, TUI_KEY_LEFT);
    CHECK(area.cursor_pos == 0);

    tui_textarea_set_text(&area, "\n\nabc\n\n");
    test_key(&desktop, TUI_KEY_DOWN);
    test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 2);
    test_key(&desktop, TUI_KEY_DOWN);
    test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 7);
}

static void test_ta_viewport(void)
{
    char buf[256];
    TuiDesktop desktop;
    TuiTextArea area;
    int i;

    ta_setup(&desktop, &area, buf, 256, 10, 3,
             "1\n2\n3\n4\n5\n6\n7\n8\n9");
    CHECK(area.top_line == 0 && area.left_col == 0);
    CHECK(vis(&area.vscroll) && !vis(&area.hscroll));

    for (i = 0; i < 4; ++i)
        test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.top_line == 2);
    CHECK(tui_scrollbar_get_value(&area.vscroll) == 2);
    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == '3');
    CHECK(test_cursor_y == 4);

    for (i = 0; i < 4; ++i)
        test_key(&desktop, TUI_KEY_UP);
    CHECK(area.top_line == 0);

    /* Page keys move viewport and cursor. */
    test_key(&desktop, TUI_KEY_PAGEDOWN);
    CHECK(area.cursor_pos == 6);
    CHECK(area.top_line == 3);
    test_key(&desktop, TUI_KEY_PAGEUP);
    CHECK(area.top_line == 0);

    /* Horizontal. */
    ta_setup(&desktop, &area, buf, 256, 8, 4,
             "0123456789ABCDEF");
    CHECK(vis(&area.hscroll) && !vis(&area.vscroll));
    test_key(&desktop, TUI_KEY_END);
    CHECK(area.left_col == 9);
    CHECK(tui_scrollbar_get_value(&area.hscroll) == 9);
    test_key(&desktop, TUI_KEY_HOME);
    CHECK(area.left_col == 0);

    /* Resize normalizes the viewport. */
    ta_setup(&desktop, &area, buf, 256, 10, 3,
             "1\n2\n3\n4\n5\n6\n7\n8\n9");
    test_key(&desktop, TUI_KEY_PAGEDOWN);
    test_key(&desktop, TUI_KEY_PAGEDOWN);
    CHECK(area.top_line > 0);
    area.control.height = 20;
    tui_draw(&desktop);
    CHECK(area.top_line == 0);
    CHECK(!vis(&area.vscroll));
    area.control.height = 2;
    tui_draw(&desktop);
    CHECK(vis(&area.vscroll));
}

static void test_ta_scrollbars(void)
{
    char buf[256];
    TuiDesktop desktop;
    TuiTextArea area;
    TuiEvent event;

    ta_setup(&desktop, &area, buf, 256, 10, 4, "ab\ncd");
    CHECK(!vis(&area.vscroll) && !vis(&area.hscroll));
    CHECK(!(area.vscroll.control.flags & TUI_TABSTOP));
    CHECK(!(area.hscroll.control.flags & TUI_TABSTOP));
    CHECK(area.vscroll.command == TUI_CMD_NONE);

    /* Vertical only. */
    ta_setup(&desktop, &area, buf, 256, 10, 3, "1\n2\n3\n4");
    CHECK(vis(&area.vscroll) && !vis(&area.hscroll));
    CHECK(area.vscroll.control.x == 9 && area.vscroll.control.height == 3);
    CHECK(area.vscroll.max == 4 && area.vscroll.page == 3);

    /* Vertical appears and makes a 10 char line not fit. */
    ta_setup(&desktop, &area, buf, 256, 10, 3, "0123456789\n2\n3\n4");
    CHECK(vis(&area.vscroll) && vis(&area.hscroll));
    CHECK(area.hscroll.max == 10 && area.hscroll.page == 9);
    CHECK(area.vscroll.control.height == 2);
    CHECK(area.hscroll.control.width == 9);
    tui_draw(&desktop);
    CHECK(test_cell_chars[4][11] == ' ');
    CHECK(test_cell_chars[3][11] == TUI_CH_DOWN_TRIANGLE);
    CHECK(test_cell_chars[4][2] == TUI_CH_LEFT_TRIANGLE);
    CHECK(test_cell_chars[4][10] == TUI_CH_RIGHT_TRIANGLE);

    /* Horizontal appears and makes the vertical needed. */
    ta_setup(&desktop, &area, buf, 256, 10, 3, "0123456789ABC\n2\n3");
    CHECK(vis(&area.vscroll) && vis(&area.hscroll));
    CHECK(area.vscroll.page == 2);

    /* Scroll bar drives the viewport without commands. */
    ta_setup(&desktop, &area, buf, 256, 10, 4,
             "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");
    tui_draw(&desktop);
    CHECK(test_mouse_action(&desktop, 11, 5, TUI_MOUSE_DOWN, &event));
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(area.top_line == 1);
    CHECK(area.cursor_pos == 0);
    CHECK(desktop.capture == 0);

    /* Thumb drag keeps the capture, also outside of the control. */
    area.top_line = 0;
    tui_draw(&desktop);
    CHECK(test_mouse_action(&desktop, 11, 3, TUI_MOUSE_DOWN, &event));
    CHECK(desktop.capture == &area.vscroll.control);
    CHECK(test_mouse_action(&desktop, 11, 4, TUI_MOUSE_MOVE, &event));
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(area.top_line == 8);
    CHECK(test_mouse_action(&desktop, 60, 22, TUI_MOUSE_UP, &event));
    CHECK(desktop.capture == 0);
    CHECK(area.cursor_pos == 0);
    CHECK(tui_scrollbar_get_value(&area.vscroll) == area.top_line);

    /* Horizontal bar: arrow click and drag. */
    ta_setup(&desktop, &area, buf, 256, 10, 4,
             "0123456789ABCDEFGHIJ");
    tui_draw(&desktop);
    CHECK(test_mouse_action(&desktop, 11, 5, TUI_MOUSE_DOWN, &event));
    CHECK(area.left_col == 1);
    CHECK(area.cursor_pos == 0);
    CHECK(test_mouse_action(&desktop, 2, 5, TUI_MOUSE_DOWN, &event));
    CHECK(area.left_col == 0);
    CHECK(test_mouse_action(&desktop, 3, 5, TUI_MOUSE_DOWN, &event));
    CHECK(desktop.capture == &area.hscroll.control);
    CHECK(test_mouse_action(&desktop, 11, 5, TUI_MOUSE_MOVE, &event));
    CHECK(area.left_col == 10);
    CHECK(test_mouse_action(&desktop, 60, 22, TUI_MOUSE_UP, &event));
    CHECK(area.cursor_pos == 0);

    /* Resize can make bars disappear/appear. */
    area.control.width = 30;
    tui_draw(&desktop);
    CHECK(!vis(&area.hscroll));
    area.control.width = 5;
    tui_draw(&desktop);
    CHECK(vis(&area.hscroll));

    /* Scroll bars are not in the tab chain. */
    ta_setup(&desktop, &area, buf, 256, 10, 3, "1\n2\n3\n4");
    test_key(&desktop, TUI_KEY_TAB);
    CHECK(desktop.focused == &area.control);
}

static void test_ta_mouse(void)
{
    char buf[128];
    TuiDesktop desktop;
    TuiTextArea area;
    TuiEvent event;

    ta_setup(&desktop, &area, buf, 128, 10, 4, "abcd\nef\n\nghij");

    CHECK(test_mouse_action(&desktop, 3, 2, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 1);
    CHECK(test_mouse_action(&desktop, 2, 3, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 5);
    /* Past the end of a line. */
    CHECK(test_mouse_action(&desktop, 9, 3, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 7);
    CHECK(test_mouse_action(&desktop, 5, 4, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 8);
    CHECK(test_mouse_action(&desktop, 4, 5, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 11);

    /* Below the document goes to its end. */
    tui_textarea_set_text(&area, "ab\ncd");
    CHECK(test_mouse_action(&desktop, 2, 5, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 5);

    /* Scrolled viewport. */
    ta_setup(&desktop, &area, buf, 128, 10, 3, "a\nb\nc\nd\ne\nfgh");
    test_key(&desktop, TUI_KEY_END);
    test_key(&desktop, TUI_KEY_PAGEDOWN);
    CHECK(area.top_line == 3);
    tui_draw(&desktop);
    CHECK(test_mouse_action(&desktop, 3, 2, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 7);
    CHECK(area.top_line == 3);

    /* Clicks on the scroll bar strip and the corner keep the cursor. */
    ta_setup(&desktop, &area, buf, 128, 10, 3,
             "a\nb\nc\nd\n0123456789ABC");
    tui_draw(&desktop);
    CHECK(vis(&area.vscroll) && vis(&area.hscroll));
    CHECK(test_mouse_action(&desktop, 11, 3, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 0);
    CHECK(test_mouse_action(&desktop, 11, 4, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 0);
    CHECK(test_mouse_action(&desktop, 5, 4, TUI_MOUSE_DOWN, &event));
    CHECK(area.cursor_pos == 0);
}

static void test_ta_readonly(void)
{
    char buf[64];
    TuiDesktop desktop;
    TuiTextArea area;
    TuiEvent event;
    int i;

    ta_setup(&desktop, &area, buf, 64, 10, 3,
             "a\nb\nc\nd\ne\nf");
    tui_textarea_set_readonly(&area, 1);

    CHECK(test_key(&desktop, 'x'));
    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(test_key(&desktop, TUI_KEY_BACKSPACE));
    CHECK(test_key(&desktop, TUI_KEY_DELETE));
    CHECK(same(&area, "a\nb\nc\nd\ne\nf"));

    for (i = 0; i < 4; ++i)
        test_key(&desktop, TUI_KEY_DOWN);
    CHECK(area.cursor_pos == 8);
    CHECK(area.top_line == 2);
    test_key(&desktop, TUI_KEY_END);
    test_key(&desktop, TUI_KEY_HOME);
    test_key(&desktop, TUI_KEY_PAGEUP);
    CHECK(area.top_line < 2);

    tui_draw(&desktop);
    CHECK(!test_cursor_visible);

    CHECK(test_mouse_action(&desktop, 11, 4, TUI_MOUSE_DOWN, &event));
    CHECK(area.top_line == 1);
    CHECK(test_mouse_action(&desktop, 2, 2, TUI_MOUSE_DOWN, &event));
    CHECK(same(&area, "a\nb\nc\nd\ne\nf"));

    tui_textarea_set_readonly(&area, 0);
    tui_draw(&desktop);
    CHECK(test_cursor_visible);
    test_key(&desktop, 'x');
    CHECK(strlen(buf) == 12);
}

static void test_ta_degenerate_sizes(void)
{
    char buf[64];
    TuiDesktop desktop;
    TuiTextArea area;
    TuiEvent event;
    int w;
    int h;

    for (w = 0; w <= 2; ++w) {
        for (h = 0; h <= 2; ++h) {
            ta_setup(&desktop, &area, buf, 64, w, h,
                     "abc\ndef\nghi\njkl");
            tui_draw(&desktop);
            test_key(&desktop, TUI_KEY_DOWN);
            test_key(&desktop, TUI_KEY_END);
            test_key(&desktop, TUI_KEY_PAGEDOWN);
            test_key(&desktop, 'z');
            test_mouse_action(&desktop, 2, 2, TUI_MOUSE_DOWN, &event);
            tui_draw(&desktop);
            CHECK(area.top_line >= 0 && area.left_col >= 0);
            CHECK(area.cursor_pos >= 0 &&
                  area.cursor_pos <= (int)strlen(buf));
            CHECK(area.vscroll.control.width >= 0);
            CHECK(area.hscroll.control.height >= 0);
        }
    }
}

void test_textarea_suite(void)
{
    test_run_case("textarea text", test_ta_text);
    test_run_case("textarea editing", test_ta_editing);
    test_run_case("textarea cursor", test_ta_cursor);
    test_run_case("textarea viewport", test_ta_viewport);
    test_run_case("textarea scrollbars", test_ta_scrollbars);
    test_run_case("textarea mouse", test_ta_mouse);
    test_run_case("textarea readonly", test_ta_readonly);
    test_run_case("textarea degenerate sizes", test_ta_degenerate_sizes);
}
