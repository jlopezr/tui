#include "tui.h"

/*
 * ------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------
 */

static int tui_strlen(const char *s)
{
    int n;

    n = 0;

    if (s == 0)
        return 0;

    while (*s != 0) {
        ++n;
        ++s;
    }

    return n;
}

static int tui_max(int a, int b)
{
    return a > b ? a : b;
}

static int tui_min(int a, int b)
{
    return a < b ? a : b;
}


/*
 * ------------------------------------------------------------
 * Drawing primitives
 * ------------------------------------------------------------
 */

static void tui_putc(TuiDraw *d, int x, int y, int ch, int attr)
{
    int sx;
    int sy;

    sx = d->ox + x;
    sy = d->oy + y;

    if (sx < d->x1 || sx >= d->x2 ||
        sy < d->y1 || sy >= d->y2)
        return;

    tui_console_cell(sx, sy, ch, attr);
}

static void tui_text(TuiDraw *d, int x, int y,
                     const char *s, int attr)
{
    if (s == 0)
        return;

    while (*s != 0) {
        tui_putc(d, x, y, (unsigned char)*s, attr);
        ++x;
        ++s;
    }
}

static void tui_fill(TuiDraw *d, int x, int y,
                     int w, int h, int ch, int attr)
{
    int xx;
    int yy;

    for (yy = 0; yy < h; ++yy) {
        for (xx = 0; xx < w; ++xx) {
            tui_putc(d, x + xx, y + yy, ch, attr);
        }
    }
}

static void tui_box(TuiDraw *d, int w, int h, int attr)
{
    int x;
    int y;

    if (w < 2 || h < 2)
        return;

    tui_putc(d, 0,     0,     TUI_CH_TL, attr);
    tui_putc(d, w - 1, 0,     TUI_CH_TR, attr);
    tui_putc(d, 0,     h - 1, TUI_CH_BL, attr);
    tui_putc(d, w - 1, h - 1, TUI_CH_BR, attr);

    for (x = 1; x < w - 1; ++x) {
        tui_putc(d, x, 0,     TUI_CH_HLINE, attr);
        tui_putc(d, x, h - 1, TUI_CH_HLINE, attr);
    }

    for (y = 1; y < h - 1; ++y) {
        tui_putc(d, 0,     y, TUI_CH_VLINE, attr);
        tui_putc(d, w - 1, y, TUI_CH_VLINE, attr);
    }
}

/*
 * ------------------------------------------------------------
 * Base control
 * ------------------------------------------------------------
 */

static void tui_control_init(TuiControl *control,
                             const TuiClass *cls,
                             int x, int y,
                             int width, int height,
                             int flags)
{
    control->cls = cls;

    control->parent = 0;
    control->first = 0;
    control->last = 0;
    control->next = 0;
    control->prev = 0;

    control->x = x;
    control->y = y;
    control->width = width;
    control->height = height;

    control->flags = flags;
}

void tui_add(TuiControl *parent, TuiControl *child)
{
    if (child->parent != 0)
        return;

    child->parent = parent;

    child->prev = parent->last;
    child->next = 0;

    if (parent->last != 0)
        parent->last->next = child;
    else
        parent->first = child;

    parent->last = child;
}

void tui_remove(TuiControl *control)
{
    TuiControl *parent;

    parent = control->parent;

    if (parent == 0)
        return;

    if (control->prev != 0)
        control->prev->next = control->next;
    else
        parent->first = control->next;

    if (control->next != 0)
        control->next->prev = control->prev;
    else
        parent->last = control->prev;

    control->parent = 0;
    control->prev = 0;
    control->next = 0;
}

/*
 * ------------------------------------------------------------
 * Classes
 * ------------------------------------------------------------
 */

static void desktop_draw(TuiControl *control, TuiDraw *draw);
static int desktop_event(TuiControl *control, TuiEvent *event);

static void window_draw(TuiControl *control, TuiDraw *draw);
static int window_event(TuiControl *control, TuiEvent *event);

static void label_draw(TuiControl *control, TuiDraw *draw);
static int label_event(TuiControl *control, TuiEvent *event);

static void button_draw(TuiControl *control, TuiDraw *draw);
static int button_event(TuiControl *control, TuiEvent *event);

static void popup_draw(TuiControl *control, TuiDraw *draw);
static int  popup_event(TuiControl *control, TuiEvent *event);

static void menubar_draw(TuiControl *control, TuiDraw *draw);
static int  menubar_event(TuiControl *control, TuiEvent *event);

static void statusbar_draw(TuiControl *control, TuiDraw *draw);
static int  statusbar_event(TuiControl *control, TuiEvent *event);

static const TuiClass desktop_class = {
    desktop_draw,
    desktop_event
};

static const TuiClass window_class = {
    window_draw,
    window_event
};

static const TuiClass label_class = {
    label_draw,
    label_event
};

static const TuiClass button_class = {
    button_draw,
    button_event
};

static const TuiClass menubar_class = {
    menubar_draw,
    menubar_event
};

static const TuiClass popup_class = {
    popup_draw,
    popup_event
};

static const TuiClass statusbar_class = {
    statusbar_draw,
    statusbar_event
};

/*
 * ------------------------------------------------------------
 * Desktop
 * ------------------------------------------------------------
 */

static void desktop_draw(TuiControl *control, TuiDraw *draw)
{
    tui_fill(draw,
             0, 0,
             control->width,
             control->height,
             ' ',
             TUI_ATTR_DESKTOP);
}

static int desktop_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;

    return 0;
}

void tui_desktop_init(TuiDesktop *desktop)
{
    tui_control_init(&desktop->control,
                     &desktop_class,
                     0, 0,
                     tui_console_width(),
                     tui_console_height(),
                     TUI_VISIBLE | TUI_ENABLED);

    desktop->focused = 0;
    desktop->capture = 0;
}

void tui_desktop_set_focus(TuiDesktop *desktop,
                           TuiControl *control)
{
    desktop->focused = control;
}

TuiControl *tui_desktop_get_focus(TuiDesktop *desktop)
{
    return desktop->focused;
}

void tui_desktop_set_capture(TuiDesktop *desktop,
                             TuiControl *control)
{
    desktop->capture = control;
}

void tui_desktop_clear_capture(TuiDesktop *desktop)
{
    desktop->capture = 0;
}


/*
 * ------------------------------------------------------------
 * Window
 * ------------------------------------------------------------
 */

static void window_draw(TuiControl *control, TuiDraw *draw)
{
    TuiWindow *window;
    int attr;
    int title_len;
    int title_x;

    window = (TuiWindow *)control;

    attr = TUI_ATTR_WINDOW;

    tui_fill(draw,
             0, 0,
             control->width,
             control->height,
             ' ',
             attr);

    tui_box(draw,
            control->width,
            control->height,
            attr);

    if (window->title != 0) {
        title_len = tui_strlen(window->title);
        title_x = (control->width - title_len - 2) / 2;

        if (title_x < 1)
            title_x = 1;

        tui_putc(draw, title_x, 0, ' ', attr);

        tui_text(draw,
                 title_x + 1,
                 0,
                 window->title,
                 attr);

        tui_putc(draw,
                 title_x + title_len + 1,
                 0,
                 ' ',
                 attr);
    }
}

static int window_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;

    return 0;
}

void tui_window_init(TuiWindow *window,
                     int x, int y,
                     int width, int height,
                     const char *title)
{
    tui_control_init(&window->control,
                     &window_class,
                     x, y,
                     width, height,
                     TUI_VISIBLE | TUI_ENABLED);

    window->title = title;
}


/*
 * ------------------------------------------------------------
 * Label
 * ------------------------------------------------------------
 */

static void label_draw(TuiControl *control, TuiDraw *draw)
{
    TuiLabel *label;

    label = (TuiLabel *)control;

    tui_text(draw,
             0, 0,
             label->text,
             TUI_ATTR_LABEL);
}

static int label_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;

    return 0;
}

void tui_label_init(TuiLabel *label,
                    int x, int y,
                    const char *text)
{
    tui_control_init(&label->control,
                     &label_class,
                     x, y,
                     tui_strlen(text), 1,
                     TUI_VISIBLE | TUI_ENABLED);

    label->text = text;
}

void tui_label_set_text(TuiLabel *label, const char *text)
{
    label->text = text;
    label->control.width = tui_strlen(text);
}


/*
 * ------------------------------------------------------------
 * Desktop lookup / focus
 * ------------------------------------------------------------
 */

static TuiDesktop *tui_find_desktop(TuiControl *control)
{
    TuiControl *c;

    c = control;

    while (c != 0) {
        if (c->cls == &desktop_class)
            return (TuiDesktop *)c;

        c = c->parent;
    }

    return 0;
}

static int tui_control_has_focus(TuiControl *control)
{
    TuiDesktop *desktop;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return 0;

    return desktop->focused == control;
}


/*
 * ------------------------------------------------------------
 * Button
 * ------------------------------------------------------------
 */

static void button_draw(TuiControl *control, TuiDraw *draw)
{
    TuiButton *button;
    int attr;
    int len;
    int x;

    button = (TuiButton *)control;

    if (tui_control_has_focus(control))
        attr = TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);
    else
        attr = TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);

    tui_fill(draw,
             0, 0,
             control->width, 1,
             ' ',
             attr);

    len = tui_strlen(button->text);

    x = (control->width - len - 2) / 2;

    if (x < 0)
        x = 0;

    tui_putc(draw, x, 0, '[', attr);

    tui_text(draw,
             x + 1, 0,
             button->text,
             attr);

    tui_putc(draw,
             x + len + 1,
             0,
             ']',
             attr);
}

static int button_event(TuiControl *control, TuiEvent *event)
{
    TuiButton *button;

    button = (TuiButton *)control;

    if (event->type == TUI_EV_KEY &&
        event->key == TUI_KEY_ENTER) {

        event->type = TUI_EV_COMMAND;
        event->command = button->command;
        event->source = control;

        return 1;
    }

    return 0;
}

void tui_button_init(TuiButton *button,
                     int x, int y,
                     int width,
                     const char *text,
                     int command)
{
    tui_control_init(&button->control,
                     &button_class,
                     x, y,
                     width, 1,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    button->text = text;
    button->command = command;
}

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

            tui_putc(draw, 0, i + 1, '+', normal_attr);

            for (x = 1; x < control->width - 1; ++x)
                tui_putc(draw, x, i + 1, '-', normal_attr);

            tui_putc(draw,
                     control->width - 1,
                     i + 1,
                     '+',
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


static int popup_event(TuiControl *control, TuiEvent *event)
{
    TuiPopupMenu *popup;
    TuiMenuBar *bar;
    TuiMenuItem *item;

    popup = (TuiPopupMenu *)control;
    bar = popup->owner;

    if (event->type != TUI_EV_KEY)
        return 0;

    switch (event->key) {

    case TUI_KEY_UP:

        popup->selected =
            tui_menu_next_selectable(
                popup->menu,
                popup->selected,
                -1);

        return 1;

    case TUI_KEY_DOWN:

        popup->selected =
            tui_menu_next_selectable(
                popup->menu,
                popup->selected,
                1);

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

        if (popup->selected < 0)
            return 1;

        item =
            &popup->menu->items[popup->selected];

        if (!tui_menuitem_selectable(item))
            return 1;

        /*
         * Close first. The command then returns to the
         * application in the same TuiEvent.
         */
        tui_menubar_close(bar);

        event->type = TUI_EV_COMMAND;
        event->command = item->command;
        event->source = control;

        return 1;
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

static int menubar_event(TuiControl *control,
                         TuiEvent *event)
{
    TuiMenuBar *bar;

    bar = (TuiMenuBar *)control;

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
        tui_console_width(),
        1,
        TUI_VISIBLE | TUI_ENABLED | TUI_GLOBAL);

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

/*
 * ------------------------------------------------------------
 * Status bar
 * ------------------------------------------------------------
 */

static void statusbar_draw(TuiControl *control, TuiDraw *draw)
{
    TuiStatusBar *bar;
    TuiStatusItem *item;
    int attr;
    int x;
    int i;
    int len;
    int status_len;
    int status_x;

    bar = (TuiStatusBar *)control;

    attr = TUI_ATTR_STATUSBAR;

    /*
     * Fill complete status bar.
     */
    tui_fill(draw,
             0, 0,
             control->width, 1,
             ' ',
             attr);

    /*
     * Draw command hints from left to right.
     *
     * For now the text contains only the description.
     * The key name is generated separately.
     */
    x = 1;

    for (i = 0; i < bar->count; ++i) {
        item = &bar->items[i];

        /*
         * Leave a little separation between items.
         */
        if (i != 0)
            x += 2;

        /*
         * Draw function key.
         */
        if (item->key >= TUI_KEY_F1 &&
            item->key <= TUI_KEY_F12) {

            int fn;

            fn = item->key - TUI_KEY_F1 + 1;

            tui_putc(draw, x, 0, 'F', attr);
            ++x;

            if (fn >= 10) {
                tui_putc(draw,
                         x, 0,
                         '0' + (fn / 10),
                         attr);
                ++x;
            }

            tui_putc(draw,
                     x, 0,
                     '0' + (fn % 10),
                     attr);
            ++x;

            tui_putc(draw, x, 0, ' ', attr);
            ++x;
        }

        /*
         * Description.
         */
        if (item->text != 0) {
            len = tui_strlen(item->text);

            tui_text(draw,
                     x, 0,
                     item->text,
                     attr);

            x += len;
        }
    }

    /*
     * Status text aligned to the right.
     */
    if (bar->status != 0) {
        status_len = tui_strlen(bar->status);

        status_x =
            control->width - status_len - 1;

        /*
         * Do not overwrite the left-side items.
         */
        if (status_x > x) {
            tui_text(draw,
                     status_x, 0,
                     bar->status,
                     attr);
        }
    }
}


static int statusbar_event(TuiControl *control,
                           TuiEvent *event)
{
    /*
     * For now StatusBar is display-only.
     *
     * Later mouse clicks may generate the command associated
     * with a TuiStatusItem.
     */
    (void)control;
    (void)event;

    return 0;
}


void tui_statusbar_init(TuiStatusBar *bar,
                        TuiStatusItem *items,
                        int count)
{
    tui_control_init(
        &bar->control,
        &statusbar_class,
        0,
        tui_console_height() - 1,
        tui_console_width(),
        1,
        TUI_VISIBLE | TUI_ENABLED);

    bar->items = items;
    bar->count = count;
    bar->status = 0;
}


void tui_statusbar_set_text(TuiStatusBar *bar,
                            const char *text)
{
    bar->status = text;
}

/*
 * ------------------------------------------------------------
 * Drawing tree
 * ------------------------------------------------------------
 */

static void tui_get_child_context(TuiControl *control,
                                  TuiDraw *draw,
                                  TuiDraw *child_draw)
{
    int left;
    int top;
    int right;
    int bottom;

    *child_draw = *draw;

    /*
     * Window has a one-character border.
     *
     * Its children therefore use the inner client area.
     *
     * Desktop and ordinary controls currently use their
     * complete area.
     */
    if (control->cls == &window_class) {
        left = draw->ox + 1;
        top = draw->oy + 1;

        right =
            draw->ox + control->width - 1;

        bottom =
            draw->oy + control->height - 1;

        child_draw->x1 =
            tui_max(child_draw->x1, left);

        child_draw->y1 =
            tui_max(child_draw->y1, top);

        child_draw->x2 =
            tui_min(child_draw->x2, right);

        child_draw->y2 =
            tui_min(child_draw->y2, bottom);

        child_draw->ox = left;
        child_draw->oy = top;
    }
}

static void tui_draw_tree(TuiControl *control,
                          TuiDraw *draw)
{
    TuiControl *child;
    TuiDraw children;
    TuiDraw cd;

    if ((control->flags & TUI_VISIBLE) == 0)
        return;

    if (control->cls != 0 &&
        control->cls->draw != 0) {
        control->cls->draw(control, draw);
    }

    if (control->first == 0)
        return;

    tui_get_child_context(control,
                          draw,
                          &children);

    child = control->first;

    while (child != 0) {
        cd = children;

        cd.ox += child->x;
        cd.oy += child->y;

        tui_draw_tree(child, &cd);

        child = child->next;
    }
}

void tui_draw(TuiDesktop *desktop)
{
    TuiDraw draw;

    desktop->control.width =
        tui_console_width();

    desktop->control.height =
        tui_console_height();

    draw.ox = 0;
    draw.oy = 0;

    draw.x1 = 0;
    draw.y1 = 0;

    draw.x2 = desktop->control.width;
    draw.y2 = desktop->control.height;

    tui_draw_tree(&desktop->control,
                  &draw);

    tui_console_present();
}


/*
 * ------------------------------------------------------------
 * Focus traversal
 * ------------------------------------------------------------
 */

static int tui_is_focusable(TuiControl *control)
{
    int mask;

    mask =
        TUI_VISIBLE |
        TUI_ENABLED |
        TUI_FOCUSABLE |
        TUI_TABSTOP;

    return (control->flags & mask) == mask;
}


/*
 * Depth-first traversal.
 *
 * Returns the control following 'control' in the tree.
 */
static TuiControl *tui_tree_next(TuiControl *root,
                                 TuiControl *control)
{
    if (control->first != 0)
        return control->first;

    while (control != 0 &&
           control != root) {

        if (control->next != 0)
            return control->next;

        control = control->parent;
    }

    return 0;
}

static TuiControl *tui_first_focusable(TuiDesktop *desktop)
{
    TuiControl *root;
    TuiControl *control;

    root = &desktop->control;

    control =
        tui_tree_next(root, root);

    while (control != 0) {
        if (tui_is_focusable(control))
            return control;

        control =
            tui_tree_next(root, control);
    }

    return 0;
}

static TuiControl *tui_next_focusable(TuiDesktop *desktop,
                                      TuiControl *current)
{
    TuiControl *root;
    TuiControl *control;

    root = &desktop->control;

    if (current == 0)
        return tui_first_focusable(desktop);

    control =
        tui_tree_next(root, current);

    while (control != 0) {
        if (tui_is_focusable(control))
            return control;

        control =
            tui_tree_next(root, control);
    }

    /*
     * End of tree: wrap around.
     */
    return tui_first_focusable(desktop);
}


/*
 * ------------------------------------------------------------
 * Event dispatch
 * ------------------------------------------------------------
 */

static int tui_dispatch_globals(TuiDesktop *desktop,
                                TuiEvent *event)
{
    TuiControl *control;

    control = desktop->control.first;

    while (control != 0) {
        if ((control->flags & TUI_GLOBAL) &&
            (control->flags & TUI_ENABLED) &&
            control->cls != 0 &&
            control->cls->event != 0) {

            if (control->cls->event(control, event))
                return 1;
        }

        control = control->next;
    }

    return 0;
}

int tui_dispatch(TuiDesktop *desktop,
                 TuiEvent *event)
{
    TuiControl *target;

    /*
     * Capture has absolute priority.
     */
    if (desktop->capture != 0) {
        target = desktop->capture;

        while (target != 0) {
            if (target->cls != 0 &&
                target->cls->event != 0) {

                if (target->cls->event(target,
                                       event))
                    return 1;
            }

            target = target->parent;
        }

        return 0;
    }

    /*
     * Global controls get the first chance.
     */
    if (tui_dispatch_globals(desktop, event))
        return 1;

    /*
     * TAB navigation.
     */
    if (event->type == TUI_EV_KEY &&
        event->key == TUI_KEY_TAB) {

        desktop->focused =
            tui_next_focusable(
                desktop,
                desktop->focused);

        return 1;
    }

    /*
     * Normal focused-control dispatch.
     */
    target = desktop->focused;

    while (target != 0) {
        if (target->cls != 0 &&
            target->cls->event != 0) {

            if (target->cls->event(target,
                                   event))
                return 1;
        }

        target = target->parent;
    }

    return 0;
}

/*
 * ------------------------------------------------------------
 * Input
 * ------------------------------------------------------------
 */

int tui_read_event(TuiEvent *event)
{
    event->type = TUI_EV_KEY;

    event->key =
        tui_console_key();

    event->command = TUI_CMD_NONE;
    event->source = 0;

    return 1;
}


/*
 * ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------
 */

int tui_init(void)
{
    return tui_console_init();
}

void tui_shutdown(void)
{
    tui_console_shutdown();
}
