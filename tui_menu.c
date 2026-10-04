#include "tui_internal.h"

static void popup_draw(TuiControl *control, TuiDraw *draw);
static int popup_event(TuiControl *control, TuiEvent *event);
static void menubar_draw(TuiControl *control, TuiDraw *draw);
static int menubar_event(TuiControl *control, TuiEvent *event);

static const TuiClass popup_class = {
    popup_draw,
    popup_event
};

static const TuiClass menubar_class = {
    menubar_draw,
    menubar_event
};

/*
 * ------------------------------------------------------------
 * Menu helpers
 * ------------------------------------------------------------
 */

static int tui_menuitem_selectable(TuiMenuItem *item)
{
    if (item->flags & TUI_MENU_SEPARATOR)
        return 0;

    if (item->flags & TUI_MENU_DISABLED)
        return 0;

    return 1;
}

static int tui_menu_first_selectable(TuiMenu *menu)
{
    int i;

    for (i = 0; i < menu->count; ++i) {
        if (tui_menuitem_selectable(&menu->items[i]))
            return i;
    }

    return -1;
}

static int tui_menu_next_selectable(TuiMenu *menu,
                                    int current,
                                    int direction)
{
    int i;
    int n;

    if (menu->count == 0)
        return -1;

    i = current;

    for (n = 0; n < menu->count; ++n) {
        i += direction;

        if (i < 0)
            i = menu->count - 1;

        if (i >= menu->count)
            i = 0;

        if (tui_menuitem_selectable(&menu->items[i]))
            return i;
    }

    return current;
}

static int tui_menu_width(TuiMenu *menu)
{
    int i;
    int width;
    int len;

    width = 0;

    for (i = 0; i < menu->count; ++i) {
        if (menu->items[i].text != 0) {
            len = tui_strlen(menu->items[i].text);

            if (len > width)
                width = len;
        }
    }

    /*
     * Border + one space at each side.
     */
    return width + 4;
}


/*
 * ------------------------------------------------------------
 * Popup menu
 * ------------------------------------------------------------
 */

static void popup_draw(TuiControl *control, TuiDraw *draw)
{
    TuiPopupMenu *popup;
    TuiMenuItem *item;
    int normal_attr;
    int selected_attr;
    int disabled_attr;
    int attr;
    int i;
    int x;

    popup = (TuiPopupMenu *)control;

    normal_attr =
        TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);

    selected_attr =
        TUI_ATTR(TUI_WHITE, TUI_BLUE);

    disabled_attr =
        TUI_ATTR(TUI_DARKGRAY, TUI_LIGHTGRAY);

    tui_fill(draw,
             0, 0,
             control->width,
             control->height,
             ' ',
             normal_attr);

    tui_box(draw,
            control->width,
            control->height,
            normal_attr);

    for (i = 0; i < popup->menu->count; ++i) {
        item = &popup->menu->items[i];

        if (item->flags & TUI_MENU_SEPARATOR) {

            tui_putc(draw, 0, i + 1,
                    TUI_CH_LTEE, normal_attr);

            for (x = 1; x < control->width - 1; ++x)
                tui_putc(draw, x, i + 1,
                        TUI_CH_HLINE, normal_attr);

            tui_putc(draw,
                    control->width - 1,
                    i + 1,
                    TUI_CH_RTEE,
                    normal_attr);

        } else {

            if (i == popup->selected)
                attr = selected_attr;
            else if (item->flags & TUI_MENU_DISABLED)
                attr = disabled_attr;
            else
                attr = normal_attr;

            /*
             * Fill the complete interior line so the selected
             * item gets a full-width highlight.
             */
            tui_fill(draw,
                     1, i + 1,
                     control->width - 2,
                     1,
                     ' ',
                     attr);

            tui_text(draw,
                     2, i + 1,
                     item->text,
                     attr);
        }
    }
}


/*
 * Forward declarations because popup events need to manipulate
 * the owning menu bar.
 */
static void tui_menubar_close(TuiMenuBar *bar);
static void tui_menubar_open(TuiMenuBar *bar);
static void tui_menubar_select(TuiMenuBar *bar, int index);
static int tui_menubar_item_x(TuiMenuBar *bar, int index);

/*
 * What changes on screen is the highlighted title (the bar's row) or the highlighted
 * item (the popup): each one says so, and the rest of the screen is not repainted.
 * Opening and closing the popup are covered by tui_add() and tui_remove().
 */
static void tui_popup_set_selected(TuiPopupMenu *popup, int item)
{
    if (item == popup->selected)
        return;

    /*
     * Only the text of the row that was highlighted and of the one that is now (item
     * i is row i + 1): the border at both ends does not change.
     */
    if (popup->selected >= 0)
        tui_invalidate_rect(&popup->control, 1, popup->selected + 1,
                            popup->control.width - 2, 1);

    if (item >= 0)
        tui_invalidate_rect(&popup->control, 1, item + 1,
                            popup->control.width - 2, 1);

    popup->selected = item;
}

/*
 * Menu title under screen (x,y), or -1.
 * Uses the same geometry as menubar_draw().
 */
static int tui_menubar_hit(TuiMenuBar *bar,
                           int screen_x, int screen_y)
{
    int lx;
    int ly;
    int i;
    int x0;

    tui_control_screen_to_local(&bar->control,
                                screen_x, screen_y,
                                &lx, &ly);

    if (ly != 0)
        return -1;

    for (i = 0; i < bar->count; ++i) {
        x0 = tui_menubar_item_x(bar, i);

        if (lx >= x0 &&
            lx < x0 + tui_strlen(bar->menus[i].text) + 2)
            return i;
    }

    return -1;
}

/*
 * Item under screen (x,y), or -1 (outside, border, no item).
 */
static int tui_popup_hit_item(TuiPopupMenu *popup,
                              int screen_x, int screen_y)
{
    int lx;
    int ly;

    tui_control_screen_to_local(&popup->control,
                                screen_x, screen_y,
                                &lx, &ly);

    if (lx < 1 || lx >= popup->control.width - 1)
        return -1;

    if (ly < 1 || ly > popup->menu->count)
        return -1;

    return ly - 1;
}

/*
 * Closes the menu and turns the item into a command.
 * Shared by ENTER and mouse click.
 */
static int tui_popup_activate(TuiPopupMenu *popup,
                              TuiEvent *event)
{
    TuiMenuItem *item;

    if (popup->selected < 0)
        return 1;

    item = &popup->menu->items[popup->selected];

    if (!tui_menuitem_selectable(item))
        return 1;

    tui_menubar_close(popup->owner);

    event->type = TUI_EV_COMMAND;
    event->command = item->command;
    event->source = &popup->control;

    return 1;
}

/*
 * The popup holds capture while open, so it receives every
 * mouse event and decides by position: popup, menu bar or
 * outside (modal).
 */
static int popup_mouse(TuiPopupMenu *popup, TuiEvent *event)
{
    TuiMenuBar *bar;
    int item;
    int title;
    int left;
    int pressed;

    bar = popup->owner;
    left = (event->mouse_action == TUI_MOUSE_MOVE) ||
           (event->mouse_buttons & TUI_MOUSE_LEFT);

    /* The second of two quick clicks arrives as DOUBLE: it is a press as well. */
    pressed = event->mouse_action == TUI_MOUSE_DOWN ||
              event->mouse_action == TUI_MOUSE_DOUBLE;

    if (!left)
        return 1;

    item = tui_popup_hit_item(popup,
                              event->mouse_x,
                              event->mouse_y);

    if (item >= 0) {
        if (!tui_menuitem_selectable(&popup->menu->items[item]))
            return 1;

        if (event->mouse_action == TUI_MOUSE_UP) {
            popup->selected = item;
            return tui_popup_activate(popup, event);
        }

        tui_popup_set_selected(popup, item);
        return 1;
    }

    title = tui_menubar_hit(bar,
                            event->mouse_x,
                            event->mouse_y);

    if (title >= 0) {
        if (pressed && title == bar->selected)
            tui_menubar_close(bar);
        else if (title != bar->selected &&
                 event->mouse_action != TUI_MOUSE_UP)
            tui_menubar_select(bar, title);

        return 1;
    }

    /* Click outside popup and titles closes; the click is consumed. */
    if (pressed)
        tui_menubar_close(bar);

    return 1;
}

static int popup_event(TuiControl *control, TuiEvent *event)
{
    TuiPopupMenu *popup;
    TuiMenuBar *bar;

    popup = (TuiPopupMenu *)control;
    bar = popup->owner;

    /* Whatever does not change anything (a separator, a key at the end) repaints nothing. */
    tui_event_done(control);

    if (event->type == TUI_EV_MOUSE)
        return popup_mouse(popup, event);

    if (event->type != TUI_EV_KEY)
        return 0;

    switch (event->key) {

    case TUI_KEY_UP:

        tui_popup_set_selected(
            popup,
            tui_menu_next_selectable(
                popup->menu,
                popup->selected,
                -1));

        return 1;

    case TUI_KEY_DOWN:

        tui_popup_set_selected(
            popup,
            tui_menu_next_selectable(
                popup->menu,
                popup->selected,
                1));

        return 1;

    case TUI_KEY_LEFT:

        tui_menubar_select(
            bar,
            bar->selected - 1);

        return 1;

    case TUI_KEY_RIGHT:

        tui_menubar_select(
            bar,
            bar->selected + 1);

        return 1;

    case TUI_KEY_ESCAPE:

        tui_menubar_close(bar);

        return 1;

    case TUI_KEY_ENTER:

        return tui_popup_activate(popup, event);
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * Menu bar
 * ------------------------------------------------------------
 */

static int tui_menubar_item_x(TuiMenuBar *bar, int index)
{
    int i;
    int x;

    x = 1;

    for (i = 0; i < index; ++i)
        x += tui_strlen(bar->menus[i].text) + 2;

    return x;
}

static void menubar_draw(TuiControl *control, TuiDraw *draw)
{
    TuiMenuBar *bar;
    int normal_attr;
    int selected_attr;
    int attr;
    int i;
    int x;
    int len;

    bar = (TuiMenuBar *)control;

    normal_attr = TUI_ATTR_MENUBAR;
    selected_attr = TUI_ATTR_MENU_SELECTED;

    tui_fill(draw,
             0, 0,
             control->width, 1,
             ' ',
             normal_attr);

    x = 1;

    for (i = 0; i < bar->count; ++i) {

        len = tui_strlen(bar->menus[i].text);

        if (bar->active && i == bar->selected)
            attr = selected_attr;
        else
            attr = normal_attr;

        tui_fill(draw,
                 x, 0,
                 len + 2, 1,
                 ' ',
                 attr);

        tui_text(draw,
                 x + 1, 0,
                 bar->menus[i].text,
                 attr);

        x += len + 2;
    }
}

void tui_menubar_activate(TuiMenuBar *bar)
{
    TuiDesktop *desktop;

    if (bar->count == 0)
        return;

    desktop =
        tui_find_desktop(&bar->control);

    if (desktop == 0)
        return;

    bar->active = 1;
    bar->selected = 0;
    tui_invalidate(&bar->control);

    /*
     * Capture the bar before a popup exists.
     */
    tui_desktop_set_capture(
        desktop,
        &bar->control);
}

static void tui_menubar_open(TuiMenuBar *bar)
{
    TuiDesktop *desktop;
    TuiMenu *menu;
    int x;

    desktop =
        tui_find_desktop(&bar->control);

    if (desktop == 0)
        return;

    if (bar->count == 0)
        return;

    menu = &bar->menus[bar->selected];

    bar->active = 1;
    tui_invalidate(&bar->control);

    bar->popup.menu = menu;
    bar->popup.selected =
        tui_menu_first_selectable(menu);

    x = tui_menubar_item_x(
        bar,
        bar->selected);

    bar->popup.control.x = x;
    bar->popup.control.y = 1;

    bar->popup.control.width =
        tui_menu_width(menu);

    bar->popup.control.height =
        menu->count + 2;

    /*
     * Popup becomes a direct child of Desktop so it is not
     * clipped by MenuBar and, because it is appended last,
     * is drawn above the normal UI.
     */
    if (bar->popup.control.parent == 0) {
        tui_add(&desktop->control,
                &bar->popup.control);
    }

    tui_desktop_set_capture(
        desktop,
        &bar->popup.control);
}

static void tui_menubar_close(TuiMenuBar *bar)
{
    TuiDesktop *desktop;

    desktop =
        tui_find_desktop(&bar->control);

    if (desktop == 0)
        return;

    if (bar->popup.control.parent != 0)
        tui_remove(&bar->popup.control);

    tui_desktop_clear_capture(desktop);

    bar->active = 0;
    tui_invalidate(&bar->control);
}

static void tui_menubar_select(TuiMenuBar *bar, int index)
{
    int was_open;

    if (bar->count == 0)
        return;

    while (index < 0)
        index += bar->count;

    while (index >= bar->count)
        index -= bar->count;

    was_open =
        bar->popup.control.parent != 0;

    if (was_open)
        tui_menubar_close(bar);

    bar->selected = index;
    tui_invalidate(&bar->control);

    if (was_open)
        tui_menubar_open(bar);
}

static int tui_menubar_accelerator(TuiMenuBar *bar,
                                   TuiEvent *event)
{
    TuiMenu *menu;
    TuiMenuItem *item;
    int i;
    int j;

    if (event->type != TUI_EV_KEY)
        return 0;

    for (i = 0; i < bar->count; ++i) {
        menu = &bar->menus[i];

        for (j = 0; j < menu->count; ++j) {
            item = &menu->items[j];

            if (item->flags & TUI_MENU_SEPARATOR)
                continue;

            if (item->flags & TUI_MENU_DISABLED)
                continue;

            if (item->key == TUI_KEY_NONE)
                continue;

            if (item->key == event->key) {
                event->type = TUI_EV_COMMAND;
                event->command = item->command;
                event->source = &bar->control;

                return 1;
            }
        }
    }

    return 0;
}

/*
 * Mouse on the bar while no popup is open (the open-popup case
 * is handled by popup_mouse because the popup owns capture).
 */
static int menubar_mouse(TuiMenuBar *bar, TuiEvent *event)
{
    int title;

    title = tui_menubar_hit(bar,
                            event->mouse_x,
                            event->mouse_y);

    if (event->mouse_action == TUI_MOUSE_DOWN &&
        (event->mouse_buttons & TUI_MOUSE_LEFT)) {

        if (title >= 0) {
            bar->selected = title;
            tui_menubar_open(bar);
            return 1;
        }

        if (bar->active) {
            tui_menubar_close(bar);
            return 1;
        }

        return 0;
    }

    /* Keyboard-activated bar without popup: hover moves the title. */
    if (event->mouse_action == TUI_MOUSE_MOVE &&
        bar->active && title >= 0) {
        if (title != bar->selected) {
            bar->selected = title;
            tui_invalidate(&bar->control);
        }
        return 1;
    }

    return bar->active;
}

static int menubar_event(TuiControl *control,
                         TuiEvent *event)
{
    TuiMenuBar *bar;

    bar = (TuiMenuBar *)control;

    /* The bar says what it changes; an accelerator turned into a command changes nothing. */
    tui_event_done(control);

    if (event->type == TUI_EV_MOUSE)
        return menubar_mouse(bar, event);

    if (event->type != TUI_EV_KEY)
        return 0;

    /*
     * F10 activates the menu bar.
     */
    if (!bar->active &&
        event->key == TUI_KEY_F10) {

        tui_menubar_activate(bar);
        return 1;
    }

    /*
     * Menu accelerators.
     */
    if (!bar->active) {
        if (tui_menubar_accelerator(bar, event))
            return 1;

        return 0;
    }

    switch (event->key) {

    case TUI_KEY_LEFT:
        tui_menubar_select(bar,
                           bar->selected - 1);
        return 1;

    case TUI_KEY_RIGHT:
        tui_menubar_select(bar,
                           bar->selected + 1);
        return 1;

    case TUI_KEY_DOWN:
    case TUI_KEY_ENTER:
        tui_menubar_open(bar);
        return 1;

    case TUI_KEY_ESCAPE:
        tui_menubar_close(bar);
        return 1;
    }

    return 0;
}

void tui_menubar_init(TuiMenuBar *bar,
                      TuiMenu *menus,
                      int count)
{
    tui_control_init(
        &bar->control,
        &menubar_class,
        0, 0,
        0, 1,
        TUI_VISIBLE | TUI_ENABLED | TUI_GLOBAL);

    bar->control.dock = TUI_DOCK_TOP;

    bar->menus = menus;
    bar->count = count;

    bar->selected = 0;
    bar->active = 0;

    tui_control_init(
        &bar->popup.control,
        &popup_class,
        0, 1,
        1, 1,
        TUI_VISIBLE | TUI_ENABLED );

    bar->popup.owner = bar;
    bar->popup.menu = 0;
    bar->popup.selected = -1;
}

