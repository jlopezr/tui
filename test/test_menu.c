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

static TuiMenuItem cov_items[] = {
    { "Open", 41, TUI_KEY_NONE, 0 },
    { 0, 0, TUI_KEY_NONE, TUI_MENU_SEPARATOR },
    { "Off", 42, TUI_KEY_F2, TUI_MENU_DISABLED },
    { "Close", 43, TUI_KEY_NONE, 0 }
};
static TuiMenuItem cov_run_items[] = {
    { "Run", 44, TUI_KEY_NONE, 0 }
};
static TuiMenu cov_menus[] = {
    { "File", cov_items, 4 },
    { "Run", cov_run_items, 1 }
};

static int cov_mouse(TuiDesktop *desktop, int x, int y,
                     int action, int buttons)
{
    TuiEvent event;

    event.type = TUI_EV_MOUSE;
    event.key = TUI_KEY_NONE;
    event.command = TUI_CMD_NONE;
    event.mouse_x = x;
    event.mouse_y = y;
    event.mouse_action = action;
    event.mouse_buttons = buttons;
    event.source = 0;

    return tui_dispatch(desktop, &event);
}

static void test_popup_mouse(void)
{
    TuiDesktop desktop;
    TuiMenuBar menubar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, cov_menus, 2);
    tui_add(&desktop.control, &menubar.control);

    CHECK(test_mouse_down(&desktop, 2, 0));
    CHECK(menubar.active);

    /* No left button: consumed, nothing changes. */
    CHECK(cov_mouse(&desktop, 3, 2, TUI_MOUSE_UP, 0));
    CHECK(menubar.popup.selected == 0);

    /* Hover moves selection; separator and disabled are ignored. */
    CHECK(cov_mouse(&desktop, 3, 5, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.popup.selected == 3);
    CHECK(cov_mouse(&desktop, 3, 3, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.popup.selected == 3);
    CHECK(cov_mouse(&desktop, 3, 4, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.popup.selected == 3);
    CHECK(cov_mouse(&desktop, 3, 4, TUI_MOUSE_UP, TUI_MOUSE_LEFT));
    CHECK(menubar.active);

    /* Border/outside items do not hit. */
    CHECK(cov_mouse(&desktop, 1, 2, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.popup.selected == 3);

    /* Hovering another title switches menu; UP on a title is ignored. */
    CHECK(cov_mouse(&desktop, 8, 0, TUI_MOUSE_UP, TUI_MOUSE_LEFT));
    CHECK(menubar.selected == 0);
    CHECK(cov_mouse(&desktop, 8, 0, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.selected == 1);
    CHECK(menubar.popup.control.parent == &desktop.control);

    /* DOWN on the selected title closes the menu. */
    CHECK(cov_mouse(&desktop, 8, 0, TUI_MOUSE_DOWN, TUI_MOUSE_LEFT));
    CHECK(!menubar.active);
    CHECK(desktop.capture == 0);

    /* DOWN outside closes; non-DOWN outside keeps it open. */
    CHECK(test_mouse_down(&desktop, 2, 0));
    CHECK(cov_mouse(&desktop, 50, 10, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.active);
    CHECK(cov_mouse(&desktop, 50, 10, TUI_MOUSE_DOWN, TUI_MOUSE_LEFT));
    CHECK(!menubar.active);

    /* Click activation on an item. */
    CHECK(test_mouse_down(&desktop, 2, 0));
    CHECK(test_mouse_action(&desktop, 3, 2, TUI_MOUSE_UP, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == 41);
}

static void test_popup_keys(void)
{
    TuiDesktop desktop;
    TuiMenuBar menubar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, cov_menus, 2);
    tui_add(&desktop.control, &menubar.control);

    CHECK(test_key(&desktop, TUI_KEY_F10));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(menubar.popup.selected == 0);

    CHECK(test_key(&desktop, TUI_KEY_UP));
    CHECK(menubar.popup.selected == 3);
    CHECK(test_key(&desktop, TUI_KEY_UP));
    CHECK(menubar.popup.selected == 0);

    CHECK(test_key(&desktop, TUI_KEY_LEFT));
    CHECK(menubar.selected == 1);
    CHECK(test_key(&desktop, TUI_KEY_RIGHT));
    CHECK(menubar.selected == 0);

    CHECK(!test_key(&desktop, TUI_KEY_F1));

    /* A command event is not handled by the popup. */
    event.type = TUI_EV_COMMAND;
    event.command = 1;
    CHECK(menubar.popup.control.cls->event(
              &menubar.popup.control, &event) == 0);

    /* ENTER with a non-selectable or missing selection is a no-op. */
    menubar.popup.selected = 2;
    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(menubar.active);
    menubar.popup.selected = -1;
    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(menubar.active);

    CHECK(test_key(&desktop, TUI_KEY_ESCAPE));
    CHECK(!menubar.active);
    CHECK(desktop.capture == 0);
}

static void test_bar_active_without_popup(void)
{
    TuiDesktop desktop;
    TuiMenuBar menubar;
    TuiEvent event;

    test_init_desktop(&desktop);
    tui_menubar_init(&menubar, cov_menus, 2);
    tui_add(&desktop.control, &menubar.control);

    /* Inactive: mouse DOWN off the titles is not handled. */
    CHECK(!cov_mouse(&desktop, 40, 0, TUI_MOUSE_DOWN, TUI_MOUSE_LEFT));
    CHECK(!cov_mouse(&desktop, 40, 0, TUI_MOUSE_MOVE, 0));
    CHECK(!test_key(&desktop, TUI_KEY_F1));

    event.type = TUI_EV_COMMAND;
    event.command = 1;
    CHECK(menubar.control.cls->event(&menubar.control, &event) == 0);

    /* Disabled and separator accelerators do not fire. */
    CHECK(!test_key(&desktop, TUI_KEY_F2));

    CHECK(test_key(&desktop, TUI_KEY_F10));
    CHECK(menubar.active);
    CHECK(menubar.popup.control.parent == 0);

    CHECK(cov_mouse(&desktop, 8, 0, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.selected == 1);
    CHECK(cov_mouse(&desktop, 40, 0, TUI_MOUSE_MOVE, 0));
    CHECK(cov_mouse(&desktop, 40, 5, TUI_MOUSE_MOVE, 0));
    CHECK(menubar.selected == 1);

    CHECK(test_key(&desktop, TUI_KEY_LEFT));
    CHECK(menubar.selected == 0);
    CHECK(test_key(&desktop, TUI_KEY_LEFT));
    CHECK(menubar.selected == 1);
    CHECK(!test_key(&desktop, TUI_KEY_F1));
    CHECK(test_key(&desktop, TUI_KEY_ESCAPE));
    CHECK(!menubar.active);

    CHECK(test_key(&desktop, TUI_KEY_F10));
    CHECK(test_key(&desktop, TUI_KEY_ENTER));
    CHECK(menubar.popup.control.parent == &desktop.control);
    CHECK(test_key(&desktop, TUI_KEY_ESCAPE));

    /* DOWN off the titles closes a keyboard-activated bar. */
    CHECK(test_key(&desktop, TUI_KEY_F10));
    CHECK(cov_mouse(&desktop, 40, 0, TUI_MOUSE_DOWN, TUI_MOUSE_LEFT));
    CHECK(!menubar.active);
}

static void test_detached_and_empty(void)
{
    static TuiMenuItem blocked[] = {
        { "A", 51, TUI_KEY_NONE, TUI_MENU_DISABLED },
        { 0, 0, TUI_KEY_NONE, TUI_MENU_SEPARATOR }
    };
    static TuiMenu blocked_menus[] = {
        { "Blocked", blocked, 2 },
        { "Empty", 0, 0 }
    };
    TuiMenuBar detached;
    TuiMenuBar empty;
    TuiDesktop desktop;

    /* Without a desktop nothing happens. */
    tui_menubar_init(&detached, cov_menus, 2);
    tui_menubar_activate(&detached);
    CHECK(!detached.active);

    test_init_desktop(&desktop);
    tui_menubar_init(&empty, 0, 0);
    tui_add(&desktop.control, &empty.control);
    tui_menubar_activate(&empty);
    CHECK(!empty.active);
    CHECK(desktop.capture == 0);
    tui_remove(&empty.control);

    /* Menus without selectable items. */
    tui_menubar_init(&detached, blocked_menus, 2);
    tui_add(&desktop.control, &detached.control);
    CHECK(test_key(&desktop, TUI_KEY_F10));
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(detached.popup.selected == -1);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(test_key(&desktop, TUI_KEY_UP));
    CHECK(test_key(&desktop, TUI_KEY_RIGHT));
    CHECK(detached.selected == 1);
    CHECK(detached.popup.control.height == 2);
    CHECK(test_key(&desktop, TUI_KEY_DOWN));
    CHECK(detached.popup.selected == -1);
    CHECK(test_key(&desktop, TUI_KEY_ESCAPE));
}

void test_menu_suite(void)
{
    test_run_case("menubar mouse activation", test_mouse_activation);
    test_run_case("menubar keyboard navigation", test_keyboard_navigation);
    test_run_case("popup mouse handling", test_popup_mouse);
    test_run_case("popup keyboard edge cases", test_popup_keys);
    test_run_case("menubar active without popup",
                  test_bar_active_without_popup);
    test_run_case("menubar detached and empty menus",
                  test_detached_and_empty);
}
