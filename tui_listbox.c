#include "tui_internal.h"

static void listbox_draw(TuiControl *control, TuiDraw *draw);
static int listbox_event(TuiControl *control, TuiEvent *event);

static const TuiClass listbox_class = {
    listbox_draw,
    listbox_event
};

/*
 * ------------------------------------------------------------
 * List box
 * ------------------------------------------------------------
 */

static void listbox_ensure_selected_visible(TuiListBox *list)
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

    tui_listbox_set_items(list, items, count);
    list->command = TUI_CMD_NONE;
}

void tui_listbox_set_items(TuiListBox *list,
                           const char **items,
                           int count)
{
    if (list == 0)
        return;

    if (items == 0 || count <= 0) {
        list->items = 0;
        list->count = 0;
        list->selected = -1;
        list->offset = 0;
        return;
    }

    list->items = items;
    list->count = count;
    list->selected = 0;
    list->offset = 0;
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

    if (list->count <= 0) {
        list->selected = -1;
        list->offset = 0;
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
    int attr;
    const char *text;

    list = (TuiListBox *)control;
    attr = tui_control_attr(control);

    tui_fill(draw,
             0, 0,
             control->width,
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
                 control->width, 1,
                 ' ', attr);

        text = list->items[index];

        if (text == 0)
            continue;

        for (x = 0;
             x < control->width && text[x] != '\0';
             ++x) {
            tui_putc(draw, x, row,
                     (unsigned char)text[x],
                     attr);
        }
    }
}

static int listbox_event(TuiControl *control, TuiEvent *event)
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

        (void)local_x;

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

