#include "tui_internal.h"

static void edit_draw(TuiControl *control, TuiDraw *draw);
static int edit_event(TuiControl *control, TuiEvent *event);
static void edit_focus_changed(TuiControl *control);

static const TuiClass edit_class = {
    edit_draw,
    edit_event,
    0,
    TUI_CLASS_OPAQUE,
    edit_focus_changed
};

/*
 * ------------------------------------------------------------
 * Edit
 * ------------------------------------------------------------
 */

void tui_edit_init(TuiEdit *edit,
                   int x,
                   int y,
                   int width,
                   char *buffer,
                   int capacity)
{
    int len;

    tui_control_init(
        &edit->control,
        &edit_class,
        x, y,
        width, 1,
        TUI_VISIBLE |
        TUI_ENABLED |
        TUI_FOCUSABLE |
        TUI_TABSTOP);

    edit->text = buffer;
    edit->capacity = capacity;

    len = 0;

    if (buffer != 0) {
        while (len < capacity - 1 &&
               buffer[len] != '\0') {
            ++len;
        }
    }

    edit->length = len;
    edit->cursor = len;

    /* The cursor starts at the end of the text: scroll so that it is in view. */
    edit->offset = width > 0 && len >= width ? len - width + 1 : 0;
}

static void edit_draw(TuiControl *control,
                      TuiDraw *draw)
{
    TuiEdit *edit;
    int attr;
    int x;
    int index;

    edit = (TuiEdit *)control;
    attr = tui_control_attr(control);

    /*
     * Clear edit area.
     */
    tui_fill(draw,
             0, 0,
             control->width, 1,
             ' ',
             attr);

    /*
     * Draw visible part of text.
     */
    for (x = 0; x < control->width; ++x) {
        index = edit->offset + x;

        if (index >= edit->length)
            break;

        tui_putc(draw,
                 x, 0,
                 (unsigned char)edit->text[index],
                 attr);
    }

    if (tui_control_has_focus(control)) {
        tui_draw_cursor(
            draw,
            edit->cursor - edit->offset,
            0);
    }
}

static void edit_insert_char(TuiEdit *edit, int ch)
{
    int i;

    if (edit->text == 0)
        return;

    if (edit->capacity <= 0)
        return;

    if (edit->length >= edit->capacity - 1)
        return;

    /*
     * Move everything from cursor onwards one position
     * to the right, including the terminating '\0'.
     */
    for (i = edit->length;
         i >= edit->cursor;
         --i) {
        edit->text[i + 1] = edit->text[i];
    }

    edit->text[edit->cursor] = (char)ch;

    ++edit->cursor;
    ++edit->length;
}

static void edit_backspace(TuiEdit *edit)
{
    int i;

    if (edit->cursor <= 0)
        return;

    /*
     * Remove character before cursor.
     * Move the rest, including '\0', one position left.
     */
    for (i = edit->cursor - 1;
         i < edit->length;
         ++i) {
        edit->text[i] = edit->text[i + 1];
    }

    --edit->cursor;
    --edit->length;
}

static void edit_delete(TuiEdit *edit)
{
    int i;

    if (edit->cursor >= edit->length)
        return;

    /*
     * Remove character at cursor.
     * Move the rest, including '\0', one position left.
     */
    for (i = edit->cursor;
         i < edit->length;
         ++i) {
        edit->text[i] = edit->text[i + 1];
    }

    --edit->length;
}

static void edit_ensure_cursor_visible(TuiEdit *edit)
{
    int width;

    width = edit->control.width;

    if (width <= 0)
        return;

    /*
     * Cursor is left of the visible area.
     */
    if (edit->cursor < edit->offset)
        edit->offset = edit->cursor;

    /*
     * Cursor is right of the visible area.
     */
    if (edit->cursor >= edit->offset + width)
        edit->offset = edit->cursor - width + 1;

    if (edit->offset < 0)
        edit->offset = 0;
}

static int edit_handle(TuiControl *control, TuiEvent *event);

/*
 * The focus moved onto or off the edit. The cursor is the only thing that looks
 * different (edit_draw asks for it while the edit has the focus), so the cell it is
 * on is all there is to draw.
 */
static void edit_focus_changed(TuiControl *control)
{
    TuiEdit *edit;
    int x;

    edit = (TuiEdit *)control;
    x = edit->cursor - edit->offset;

    if (x >= 0 && x < control->width)
        tui_invalidate_rect(control, x, 0, 1, 1);
    else
        tui_invalidate(control);
}

/*
 * Reports to the desktop what the event changed, and no more:
 *
 *   the text scrolled                       all of it
 *   a character was typed or deleted        from the first column that changed
 *   only the cursor moved                   the cell it is on
 */
static int edit_event(TuiControl *control, TuiEvent *event)
{
    TuiEdit *edit;
    int offset;
    int length;
    int cursor;
    int handled;
    int x;

    edit = (TuiEdit *)control;
    offset = edit->offset;
    length = edit->length;
    cursor = edit->cursor;

    handled = edit_handle(control, event);

    if (!handled)
        return 0;

    x = edit->cursor - edit->offset;

    if (edit->offset != offset || x < 0 || x >= control->width) {
        tui_invalidate(control);
    } else if (edit->length != length) {
        x = tui_max(0, tui_min(cursor, edit->cursor) - edit->offset);
        tui_invalidate_rect(control, x, 0, control->width - x, 1);
    } else if (edit->cursor != cursor) {
        tui_invalidate_rect(control, x, 0, 1, 1);
    } else {
        tui_event_done(control);
    }

    return 1;
}

static int edit_handle(TuiControl *control, TuiEvent *event)
{
    TuiEdit *edit;

    edit = (TuiEdit *)control;

    if (event->type == TUI_EV_MOUSE &&
        event->mouse_action == TUI_MOUSE_DOWN &&
        (event->mouse_buttons & TUI_MOUSE_LEFT)) {

        int x;
        int y;
        int pos;

        tui_control_screen_to_local(
            control,
            event->mouse_x,
            event->mouse_y,
            &x,
            &y);

        pos = edit->offset + x;

        if (pos < 0)
            pos = 0;

        if (pos > edit->length)
            pos = edit->length;

        edit->cursor = pos;
        edit_ensure_cursor_visible(edit);

        return 1;
    }

    switch (event->key) {

    case TUI_KEY_LEFT:
        if (edit->cursor > 0)
            --edit->cursor;

        edit_ensure_cursor_visible(edit);
        return 1;

    case TUI_KEY_RIGHT:
        if (edit->cursor < edit->length)
            ++edit->cursor;

        edit_ensure_cursor_visible(edit);
        return 1;

    case TUI_KEY_HOME:
        edit->cursor = 0;
        edit_ensure_cursor_visible(edit);
        return 1;

    case TUI_KEY_END:
        edit->cursor = edit->length;
        edit_ensure_cursor_visible(edit);
        return 1;

    case TUI_KEY_BACKSPACE:
        edit_backspace(edit);
        edit_ensure_cursor_visible(edit);
        return 1;

    case TUI_KEY_DELETE:
        edit_delete(edit);
        edit_ensure_cursor_visible(edit);
        return 1;
    }

    /*
     * Printable character, as the console defines it.
     */
    if (tui_console_printable(event->key)) {

        edit_insert_char(edit, event->key);
        edit_ensure_cursor_visible(edit);
        return 1;
    }

    return 0;
}

void tui_edit_set_text(TuiEdit *edit,
                       const char *text)
{
    int i;

    if (edit == 0 ||
        edit->text == 0 ||
        edit->capacity <= 0)
        return;

    tui_invalidate(&edit->control);

    i = 0;

    if (text != 0) {
        while (i < edit->capacity - 1 &&
               text[i] != '\0') {
            edit->text[i] = text[i];
            ++i;
        }
    }

    edit->text[i] = '\0';

    edit->length = i;
    edit->cursor = i;
    edit->offset = 0;
}

const char *tui_edit_get_text(TuiEdit *edit)
{
    return edit->text;
}
