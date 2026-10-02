#include "test_support.h"

static void test_draw_and_text_update(void)
{
    TuiDesktop desktop;
    TuiLabel label;

    test_init_desktop(&desktop);
    tui_label_init(&label, 2, 3, "Before");
    tui_add(&desktop.control, &label.control);

    tui_draw(&desktop);
    CHECK(test_cell_chars[3][2] == 'B');
    CHECK(test_cell_chars[3][7] == 'e');

    tui_label_set_text(&label, "After");
    tui_draw(&desktop);
    CHECK(test_cell_chars[3][2] == 'A');
    CHECK(test_cell_chars[3][6] == 'r');
}

static void test_ignores_events(void)
{
    TuiLabel label;
    TuiEvent event;

    tui_label_init(&label, 0, 0, "Text");
    event.type = TUI_EV_KEY;
    event.key = TUI_KEY_ENTER;

    CHECK(label.control.cls->event(&label.control, &event) == 0);
}

void test_label_suite(void)
{
    test_run_case("label draw and text update", test_draw_and_text_update);
    test_run_case("label ignores events", test_ignores_events);
}
