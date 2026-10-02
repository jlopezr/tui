#include "test_support.h"

static void test_mouse_activation(void)
{
    static TuiMenuItem items[] = {
        { "Run", 84, TUI_KEY_NONE, 0 }
    };
    static TuiMenu menus[] = {
        { "File", items, 1 }
    };
    TuiDesktop desktop;
    TuiMenuBar menubar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, menus, 1);
    tui_add(&desktop.control, &menubar.control);

    CHECK(test_mouse_down(&desktop, 2, 0));
    CHECK(menubar.active);
    CHECK(desktop.capture == &menubar.popup.control);
    CHECK(menubar.popup.control.parent == &desktop.control);

    CHECK(test_mouse_action(&desktop, 2, 2,
                            TUI_MOUSE_DOWN, &event));
    CHECK(event.type == TUI_EV_MOUSE);
    CHECK(test_mouse_action(&desktop, 2, 2,
                            TUI_MOUSE_UP, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 84);
    CHECK(event.source == &menubar.popup.control);
    CHECK(!menubar.active);
    CHECK(desktop.capture == 0);
    CHECK(menubar.popup.control.parent == 0);
}

static void test_keyboard_navigation(void)
{
    static TuiMenuItem file_items[] = {
        { "Open", 31, TUI_KEY_F3, 0 },
        { 0, 0, TUI_KEY_NONE, TUI_MENU_SEPARATOR },
        { "Disabled", 32, TUI_KEY_NONE, TUI_MENU_DISABLED },
        { "Close", 33, TUI_KEY_NONE, 0 }
    };
    static TuiMenuItem run_items[] = {
        { "Run", 34, TUI_KEY_NONE, 0 }
    };
    static TuiMenu menus[] = {
        { "File", file_items, 4 },
        { "Run", run_items, 1 }
    };
    TuiDesktop desktop;
    TuiMenuBar menubar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, menus, 2);
    tui_add(&desktop.control, &menubar.control);

    CHECK(test_key_event(&desktop, TUI_KEY_F3, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 31);

    CHECK(test_key(&desktop, TUI_KEY_F10));
    CHECK(menubar.active);
    CHECK(desktop.capture == &menubar.control);
    CHECK(test_key(&desktop, TUI_KEY_RIGHT));
    CHECK(menubar.selected == 1);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(desktop.capture == &menubar.popup.control);
    CHECK(menubar.popup.selected == 0);

    CHECK(test_key(&desktop, TUI_KEY_LEFT));
    CHECK(menubar.selected == 0);
    CHECK(desktop.capture == &menubar.popup.control);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(menubar.popup.selected == 3);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(menubar.popup.selected == 0);

    tui_draw(&desktop);
    CHECK(test_cell_chars[2][3] == 'O');
    CHECK(test_cell_attrs[2][3] == TUI_ATTR_MENU_SELECTED);

    CHECK(test_key_event(&desktop, TUI_KEY_ENTER, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 31);
    CHECK(!menubar.active);
    CHECK(desktop.capture == 0);
}

void test_menu_suite(void)
{
    test_run_case("menubar mouse activation", test_mouse_activation);
    test_run_case("menubar keyboard navigation", test_keyboard_navigation);
}
