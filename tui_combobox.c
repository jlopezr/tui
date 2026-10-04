#include "tui_internal.h"

#define TUI_COMBOBOX_MAX_ROWS 8

static void combobox_draw(TuiControl *control, TuiDraw *draw);
static int combobox_event(TuiControl *control, TuiEvent *event);
static void combobox_detach(TuiControl *control);

static const TuiClass combobox_class = {
    combobox_draw,
    combobox_event,
    combobox_detach,
    TUI_CLASS_OPAQUE
};

/*
 * The list of a combo box is a window of the desktop, not of the window that holds
 * the combo box, so it would stay on screen when the combo box goes away.
 */
static void combobox_detach(TuiControl *control)
{
    tui_combobox_close((TuiComboBox *)control);
}

static int combobox_attr(TuiControl *control)
{
    return tui_control_attr(control);
}

static void combobox_draw(TuiControl *control, TuiDraw *draw)
{
    TuiComboBox *combo;
    const char *text;
    int attr;
    int text_width;
    int x;

    combo = (TuiComboBox *)control;
    attr = combobox_attr(control);

    tui_fill(draw, 0, 0, control->width, 1, ' ', attr);

    if (control->width <= 0)
        return;

    tui_putc(draw, 0, 0, '[', attr);

    if (control->width > 1)
        tui_putc(draw, control->width - 1, 0, ']', attr);

    /* Points down to say "open me", up while the list is open. */
    if (control->width > 2)
        tui_putc(draw, control->width - 2, 0,
                 combo->open ? TUI_CH_UP_TRIANGLE : TUI_CH_DOWN_TRIANGLE, attr);

    if (control->width <= 3 ||
        combo->selected < 0 ||
        combo->selected >= combo->count ||
        combo->items == 0 ||
        combo->items[combo->selected] == 0)
        return;

    text = combo->items[combo->selected];
    text_width = control->width - 4;
    x = 1;

    while (*text != '\0' && x < 2 + text_width) {
        tui_putc(draw, x, 0, (unsigned char)*text, attr);
        ++text;
        ++x;
    }
}

static int combobox_clamp(int value, int minimum, int maximum)
{
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

static void combobox_close(TuiComboBox *combo, int commit,
                           TuiEvent *event)
{
    TuiDesktop *desktop;
    int changed;

    desktop = tui_find_desktop(&combo->control);

    if (!commit) {
        combo->selected = combo->original_selected;
        tui_listbox_set_selected(&combo->popup_list,
                                 combo->selected);
    } else {
        combo->selected =
            tui_listbox_get_selected(&combo->popup_list);
    }

    changed = combo->selected != combo->original_selected;
    combo->open = 0;
    combo->popup_list.scrollbar.dragging = 0;

    if (desktop != 0) {
        if (combo->popup_window.control.parent != 0)
            tui_remove(&combo->popup_window.control);

        if (desktop->capture == &combo->control)
            tui_desktop_clear_capture(desktop);

        tui_desktop_set_focus(desktop, &combo->control);
    }

    tui_invalidate(&combo->control);

    if (commit &&
        changed &&
        combo->selected != combo->original_selected &&
        combo->command != TUI_CMD_NONE) {
        event->type = TUI_EV_COMMAND;
        event->command = combo->command;
        event->source = &combo->control;
    }
}

static int combobox_open(TuiComboBox *combo)
{
    TuiDesktop *desktop;
    int screen_x;
    int screen_y;
    int popup_width;
    int popup_height;
    int max_rows;
    int rows;
    int below;
    int above;
    int popup_x;
    int popup_y;

    if (combo->open || combo->count <= 0)
        return 0;

    desktop = tui_find_desktop(&combo->control);

    if (desktop == 0)
        return 0;

    tui_control_screen_pos(&combo->control,
                           &screen_x, &screen_y);

    popup_width = combo->control.width;

    if (popup_width < 3)
        popup_width = 3;

    if (popup_width > desktop->control.width)
        popup_width = desktop->control.width;

    if (popup_width <= 0)
        return 0;

    max_rows = combo->count;

    if (max_rows > TUI_COMBOBOX_MAX_ROWS)
        max_rows = TUI_COMBOBOX_MAX_ROWS;

    below = desktop->control.height - screen_y - 1;
    above = screen_y;

    if (below >= 3) {
        rows = below - 2;
        if (rows > max_rows)
            rows = max_rows;
        popup_y = screen_y + 1;
    } else if (above >= 3) {
        rows = above - 2;
        if (rows > max_rows)
            rows = max_rows;
        popup_y = screen_y - rows - 2;
    } else {
        return 0;
    }

    if (rows <= 0)
        return 0;

    popup_height = rows + 2;
    popup_x = combobox_clamp(screen_x, 0,
                             desktop->control.width - popup_width);

    combo->popup_window.control.x = popup_x;
    combo->popup_window.control.y = popup_y;
    combo->popup_window.control.width = popup_width;
    combo->popup_window.control.height = popup_height;
    combo->popup_window.control.attr =
        tui_control_attr(&combo->control);

    combo->popup_list.control.x = 0;
    combo->popup_list.control.y = 0;
    combo->popup_list.control.width = popup_width - 2;
    if (combo->popup_list.control.width < 0)
        combo->popup_list.control.width = 0;
    combo->popup_list.control.height = rows;
    tui_listbox_set_items(&combo->popup_list,
                          combo->items, combo->count);
    tui_listbox_set_selected(&combo->popup_list,
                             combo->selected);

    combo->original_selected = combo->selected;
    combo->open = 1;

    tui_add(&desktop->control, &combo->popup_window.control);
    tui_desktop_set_capture(desktop, &combo->control);
    tui_desktop_set_focus(desktop, &combo->control);
    tui_invalidate(&combo->control);

    return 1;
}

static int combobox_popup_mouse(TuiComboBox *combo,
                                TuiEvent *event)
{
    TuiDesktop *desktop;
    TuiControl *target;
    TuiControl *scroll;
    int pressed;

    desktop = tui_find_desktop(&combo->control);

    if (desktop == 0)
        return 0;

    /* The second of two quick clicks arrives as DOUBLE: it is a press as well. */
    pressed = event->mouse_action == TUI_MOUSE_DOWN ||
              event->mouse_action == TUI_MOUSE_DOUBLE;

    scroll = &combo->popup_list.scrollbar.control;

    /*
     * The scroll bar takes the capture for its drag; the combo box
     * gets it back so it keeps owning the popup, and forwards the
     * rest of the drag itself.
     */
    if (combo->popup_list.scrollbar.dragging) {
        scroll->cls->event(scroll, event);
        return 1;
    }

    target = tui_hit_test(&desktop->control,
                          event->mouse_x,
                          event->mouse_y);

    if (target == scroll) {
        scroll->cls->event(scroll, event);

        if (combo->popup_list.scrollbar.dragging)
            tui_desktop_set_capture(desktop, &combo->control);

        return 1;
    }

    if (target == &combo->popup_list.control) {
        if (target->cls != 0 && target->cls->event != 0)
            target->cls->event(target, event);

        if (pressed) {
            combobox_close(combo, 1, event);
            return 1;
        }

        return 1;
    }

    if (pressed) {
        combobox_close(combo, 0, event);
        return 1;
    }

    return 1;
}

static int combobox_handle(TuiControl *control, TuiEvent *event);

/*
 * Handled events count as reported even when nothing here changed (a press
 * on a button, a mouse move over an open list): the controls that did change
 * something have invalidated it themselves.
 */
static int combobox_event(TuiControl *control, TuiEvent *event)
{
    int handled;

    handled = combobox_handle(control, event);

    if (handled)
        tui_event_done(control);

    return handled;
}

static int combobox_handle(TuiControl *control, TuiEvent *event)
{
    TuiComboBox *combo;
    TuiDesktop *desktop;

    combo = (TuiComboBox *)control;

    if (combo->open) {
        if (event->type == TUI_EV_MOUSE)
            return combobox_popup_mouse(combo, event);

        if (event->type != TUI_EV_KEY)
            return 0;

        if (event->key == TUI_KEY_ESCAPE) {
            combobox_close(combo, 0, event);
            return 1;
        }

        if (event->key == TUI_KEY_ENTER) {
            combobox_close(combo, 1, event);
            return 1;
        }

        if (combo->popup_list.control.cls != 0 &&
            combo->popup_list.control.cls->event != 0)
            return combo->popup_list.control.cls->event(
                &combo->popup_list.control, event);

        return 0;
    }

    if (event->type == TUI_EV_MOUSE &&
        event->mouse_action == TUI_MOUSE_DOWN &&
        (event->mouse_buttons & TUI_MOUSE_LEFT)) {
        combobox_open(combo);
        return 1;
    }

    if (event->type != TUI_EV_KEY ||
        (event->key != TUI_KEY_ENTER &&
         event->key != ' ' &&
         event->key != TUI_KEY_DOWN &&
         event->key != TUI_KEY_UP))
        return 0;

    desktop = tui_find_desktop(control);
    if (desktop != 0)
        tui_desktop_set_focus(desktop, control);

    combobox_open(combo);

    if (combo->open &&
        (event->key == TUI_KEY_DOWN ||
         event->key == TUI_KEY_UP) &&
        combo->popup_list.control.cls != 0 &&
        combo->popup_list.control.cls->event != 0)
        combo->popup_list.control.cls->event(
            &combo->popup_list.control, event);

    return 1;
}

void tui_combobox_init(TuiComboBox *combo,
                       int x,
                       int y,
                       int width,
                       const char **items,
                       int count)
{
    tui_control_init(&combo->control,
                     &combobox_class,
                     x, y,
                     width, 1,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    combo->items = 0;
    combo->count = 0;
    combo->selected = -1;
    combo->command = TUI_CMD_NONE;
    combo->open = 0;
    combo->original_selected = -1;

    tui_window_init(&combo->popup_window, 0, 0, 3, 3, 0);
    tui_listbox_init(&combo->popup_list, 1, 1, 1, 1, 0, 0);
    combo->popup_list.control.flags &=
        ~(TUI_FOCUSABLE | TUI_TABSTOP);
    tui_add(&combo->popup_window.control,
            &combo->popup_list.control);

    tui_combobox_set_items(combo, items, count);
}

void tui_combobox_set_items(TuiComboBox *combo,
                            const char **items,
                            int count)
{
    if (combo == 0)
        return;

    if (combo->open) {
        combo->selected = combo->original_selected;
        combo->open = 0;
        if (combo->popup_window.control.parent != 0)
            tui_remove(&combo->popup_window.control);
        {
            TuiDesktop *desktop;

            desktop = tui_find_desktop(&combo->control);
            if (desktop != 0 && desktop->capture == &combo->control)
                tui_desktop_clear_capture(desktop);
        }
    }

    if (items == 0 || count <= 0) {
        combo->items = 0;
        combo->count = 0;
        combo->selected = -1;
    } else {
        combo->items = items;
        combo->count = count;
        combo->selected = 0;
    }

    tui_invalidate(&combo->control);
}

int tui_combobox_get_selected(TuiComboBox *combo)
{
    return combo != 0 ? combo->selected : -1;
}

void tui_combobox_set_selected(TuiComboBox *combo, int index)
{
    if (combo == 0)
        return;

    if (combo->count <= 0) {
        combo->selected = -1;
        return;
    }

    if (index < 0)
        index = 0;
    else if (index >= combo->count)
        index = combo->count - 1;

    combo->selected = index;
    tui_invalidate(&combo->control);

    if (combo->open)
        tui_listbox_set_selected(&combo->popup_list, index);
}

void tui_combobox_set_scrollbar(TuiComboBox *combo, int enabled)
{
    if (combo != 0)
        tui_listbox_set_scrollbar(&combo->popup_list, enabled);
}

void tui_combobox_set_command(TuiComboBox *combo, int command)
{
    if (combo != 0)
        combo->command = command;
}

void tui_combobox_close(TuiComboBox *combo)
{
    TuiEvent unused;

    if (combo == 0 || !combo->open)
        return;

    /* Cancelling never turns the event into a command, so 'unused' stays unused. */
    combobox_close(combo, 0, &unused);
}
