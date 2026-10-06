#include "tui_internal.h"

static void listbox_draw(TuiControl *control, TuiDraw *draw);
static int listbox_event(TuiControl *control, TuiEvent *event);
static void listbox_focus_changed(TuiControl *control);

static const TuiClass listbox_class = {
    listbox_draw,
    listbox_event,
    0,
    TUI_CLASS_OPAQUE,
    listbox_focus_changed
};

/*
 * ------------------------------------------------------------
 * List box
 * ------------------------------------------------------------
 */

static void listbox_sync_scrollbar(TuiListBox *list);

static int listbox_content_width(const TuiListBox *list)
{
    if (list->scrollbar.control.flags & TUI_VISIBLE)
        return tui_max(0, list->control.width - 1);

    return list->control.width;
}

/* The internal scroll bar is a hidden child with a private class. */
static void listbox_scroll_draw(TuiControl *control, TuiDraw *draw)
{
    tui_scrollbar_class.draw(control, draw);
}

static int listbox_scroll_event(TuiControl *control, TuiEvent *event)
{
    TuiListBox *list;
    int handled;

    handled = tui_scrollbar_class.event(control, event);
    list = (TuiListBox *)control->parent;

    if (list != 0) {
        list->offset = tui_scrollbar_get_value(&list->scrollbar);

        /* The viewport moved: the rows have to be drawn again. */
        if (handled)
            tui_invalidate(&list->control);
    }

    return handled;
}

static void listbox_scroll_detach(TuiControl *control)
{
    tui_scrollbar_class.detach(control);
}

static const TuiClass listbox_scroll_class = {
    listbox_scroll_draw,
    listbox_scroll_event,
    listbox_scroll_detach,
    TUI_CLASS_OPAQUE
};

static void listbox_do_ensure_visible(TuiListBox *list)
{
    int height;
    int max_offset;

    if (list->count <= 0) {
        list->selected = -1;
        list->offset = 0;
        return;
    }

    if (list->selected < 0)
        list->selected = 0;
    else if (list->selected >= list->count)
        list->selected = list->count - 1;

    height = list->control.height;

    if (height <= 0) {
        list->offset = 0;
        return;
    }

    if (list->selected < list->offset)
        list->offset = list->selected;

    if (list->selected - list->offset >= height)
        list->offset = list->selected - height + 1;

    if (list->offset < 0)
        list->offset = 0;

    max_offset = list->count > height
        ? list->count - height
        : 0;

    if (list->offset > max_offset)
        list->offset = max_offset;
}

static void listbox_ensure_selected_visible(TuiListBox *list)
{
    listbox_do_ensure_visible(list);
    listbox_sync_scrollbar(list);
}

/* Single place that derives scroll bar state from the list state. */
static void listbox_sync_scrollbar(TuiListBox *list)
{
    TuiScrollBar *sb;
    int height;
    int max_offset;

    sb = &list->scrollbar;
    height = list->control.height;

    if (!list->scrollbar_enabled ||
        height <= 0 ||
        list->count <= height) {
        sb->control.flags &= ~TUI_VISIBLE;

        if (list->scrollbar_enabled)
            list->offset = 0;

        return;
    }

    max_offset = list->count - height;

    if (list->offset > max_offset)
        list->offset = max_offset;

    if (list->offset < 0)
        list->offset = 0;

    sb->control.flags |= TUI_VISIBLE;
    sb->control.x = list->control.width - 1;
    sb->control.y = 0;
    sb->control.width = 1;
    sb->control.height = height;

    tui_scrollbar_set_range(sb, 0, list->count);
    tui_scrollbar_set_page(sb, height);
    tui_scrollbar_set_value(sb, list->offset);
}

void tui_listbox_init(TuiListBox *list,
                      int x,
                      int y,
                      int width,
                      int height,
                      const char **items,
                      int count)
{
    tui_control_init(&list->control,
                     &listbox_class,
                     x, y,
                     width, height,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    tui_scrollbar_init(&list->scrollbar, 0, 0, 1,
                       TUI_VERTICAL, TUI_CMD_NONE);
    list->scrollbar.control.cls = &listbox_scroll_class;
    list->scrollbar.control.flags = TUI_ENABLED;
    list->scrollbar_enabled = 0;
    tui_add(&list->control, &list->scrollbar.control);

    tui_listbox_set_items(list, items, count);
    list->command = TUI_CMD_NONE;
}

void tui_listbox_set_items(TuiListBox *list,
                           const char **items,
                           int count)
{
    if (list == 0)
        return;

    tui_invalidate(&list->control);

    if (items == 0 || count <= 0) {
        list->items = 0;
        list->count = 0;
        list->selected = -1;
        list->offset = 0;
        listbox_sync_scrollbar(list);
        return;
    }

    list->items = items;
    list->count = count;
    list->selected = 0;
    list->offset = 0;
    listbox_sync_scrollbar(list);
}

void tui_listbox_set_scrollbar(TuiListBox *list, int enabled)
{
    if (list == 0)
        return;

    tui_invalidate(&list->control);

    list->scrollbar_enabled = enabled != 0;
    listbox_sync_scrollbar(list);
}

int tui_listbox_get_selected(TuiListBox *list)
{
    if (list == 0)
        return -1;

    return list->selected;
}

void tui_listbox_set_selected(TuiListBox *list,
                              int index)
{
    if (list == 0)
        return;

    tui_invalidate(&list->control);

    if (list->count <= 0) {
        list->selected = -1;
        list->offset = 0;
        listbox_sync_scrollbar(list);
        return;
    }

    if (index < 0)
        index = 0;
    else if (index >= list->count)
        index = list->count - 1;

    list->selected = index;
    listbox_ensure_selected_visible(list);
}

void tui_listbox_set_command(TuiListBox *list,
                             int command)
{
    if (list != 0)
        list->command = command;
}

static int listbox_activate(TuiListBox *list,
                            TuiControl *control,
                            TuiEvent *event)
{
    if (list->count <= 0 ||
        list->selected < 0 ||
        list->selected >= list->count ||
        list->command == TUI_CMD_NONE)
        return 0;

    event->type = TUI_EV_COMMAND;
    event->command = list->command;
    event->source = control;

    return 1;
}

static int listbox_ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 'a';

    return ch;
}

static int listbox_type_to_select(TuiListBox *list,
                                  int key)
{
    int index;
    int i;
    int first;

    if (!((key >= 'A' && key <= 'Z') ||
          (key >= 'a' && key <= 'z') ||
          (key >= '0' && key <= '9')))
        return 0;

    if (list->count <= 0)
        return 1;

    if (list->selected >= 0 &&
        list->selected < list->count)
        index = list->selected;
    else
        index = -1;

    for (i = 0; i < list->count; ++i) {
        ++index;

        if (index >= list->count)
            index = 0;

        if (list->items[index] == 0 ||
            list->items[index][0] == '\0')
            continue;

        first = (unsigned char)list->items[index][0];

        if (listbox_ascii_lower(first) ==
            listbox_ascii_lower(key)) {
            list->selected = index;
            listbox_ensure_selected_visible(list);
            return 1;
        }
    }

    return 1;
}

static void listbox_draw(TuiControl *control, TuiDraw *draw)
{
    TuiListBox *list;
    int row;
    int index;
    int x;
    int width;
    int attr;
    const char *text;

    list = (TuiListBox *)control;
    attr = tui_control_attr(control);

    listbox_sync_scrollbar(list);
    width = listbox_content_width(list);

    tui_fill(draw,
             0, 0,
             width,
             control->height,
             ' ',
             attr);

    for (row = 0; row < control->height; ++row) {
        index = list->offset + row;

        if (index < 0 || index >= list->count)
            continue;

        if (index == list->selected) {
            if (tui_control_has_focus(control))
                attr = TUI_ATTR_MENU_SELECTED;
            else
                attr = TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);
        } else {
            attr = tui_control_attr(control);
        }

        tui_fill(draw, 0, row,
                 width, 1,
                 ' ', attr);

        text = list->items[index];

        if (text == 0)
            continue;

        for (x = 0;
             x < width && text[x] != '\0';
             ++x) {
            tui_putc(draw, x, row,
                     (unsigned char)text[x],
                     attr);
        }
    }
}

/*
 * The focus moved onto or off the list: the selected row is the only one drawn
 * differently with it (see listbox_draw), so that row is all there is to draw.
 */
static void listbox_focus_changed(TuiControl *control)
{
    TuiListBox *list;
    int row;

    list = (TuiListBox *)control;
    row = list->selected - list->offset;

    if (list->selected >= 0 && row >= 0 && row < control->height)
        tui_invalidate_rect(control, 0, row, listbox_content_width(list), 1);
    else
        tui_event_done(control);
}

static int listbox_handle(TuiControl *control, TuiEvent *event);

/* Whatever the control handled may have changed what it shows. */
static int listbox_event(TuiControl *control, TuiEvent *event)
{
    int handled;

    handled = listbox_handle(control, event);

    if (handled)
        tui_invalidate(control);

    return handled;
}

static int listbox_handle(TuiControl *control, TuiEvent *event)
{
    TuiListBox *list;
    int local_x;
    int local_y;
    int index;
    int height;

    list = (TuiListBox *)control;

    if (event->type == TUI_EV_MOUSE &&
        (event->mouse_action == TUI_MOUSE_DOWN ||
         event->mouse_action == TUI_MOUSE_DOUBLE) &&
        (event->mouse_buttons & TUI_MOUSE_LEFT)) {
        tui_control_screen_to_local(control,
                                    event->mouse_x,
                                    event->mouse_y,
                                    &local_x,
                                    &local_y);

        /* The scroll bar column never selects a row. */
        if (local_x >= listbox_content_width(list))
            return 1;

        if (local_y < 0 || local_y >= control->height)
            return 0;

        index = list->offset + local_y;

        if (index >= 0 && index < list->count) {
            tui_listbox_set_selected(list, index);

            if (event->mouse_action == TUI_MOUSE_DOUBLE) {
                listbox_activate(list, control, event);
                return 1;
            }
        }

        return 1;
    }

    if (event->type != TUI_EV_KEY)
        return 0;

    if (event->key == TUI_KEY_ENTER) {
        listbox_activate(list, control, event);
        return 1;
    }

    height = control->height;

    switch (event->key) {
    case TUI_KEY_UP:
        if (list->count > 0 && list->selected > 0)
            --list->selected;
        listbox_ensure_selected_visible(list);
        return 1;

    case TUI_KEY_DOWN:
        if (list->count > 0 &&
            list->selected < list->count - 1)
            ++list->selected;
        listbox_ensure_selected_visible(list);
        return 1;

    case TUI_KEY_HOME:
        if (list->count > 0)
            list->selected = 0;
        listbox_ensure_selected_visible(list);
        return 1;

    case TUI_KEY_END:
        if (list->count > 0)
            list->selected = list->count - 1;
        listbox_ensure_selected_visible(list);
        return 1;

    case TUI_KEY_PAGEUP:
        if (list->count > 0) {
            if (height >= list->selected)
                list->selected = 0;
            else
                list->selected -= height;
        }
        listbox_ensure_selected_visible(list);
        return 1;

    case TUI_KEY_PAGEDOWN:
        if (list->count > 0) {
            if (height >= list->count - 1 - list->selected)
                list->selected = list->count - 1;
            else
                list->selected += height;
        }
        listbox_ensure_selected_visible(list);
        return 1;
    }

    return listbox_type_to_select(list, event->key);
}

