#include "tui_internal.h"

static void editor_draw(TuiControl *control, TuiDraw *draw);
static int editor_event(TuiControl *control, TuiEvent *event);

static const TuiClass editor_class = {
    editor_draw,
    editor_event,
    0,
    TUI_CLASS_OPAQUE
};

/*
 * ------------------------------------------------------------
 * Editor
 *
 * Everything is derived from the model through its public
 * operations only; lines are found by walking the characters.
 * Nothing is cached on purpose.
 * ------------------------------------------------------------
 */

static int editor_length(const TuiEditor *editor)
{
    return tui_text_model_length(editor->model);
}

/* Character at pos, or '\0' outside the text. */
static char editor_at(const TuiEditor *editor, int pos)
{
    char ch;

    if (pos < 0 ||
        tui_text_model_read(editor->model, pos, &ch, 1) != 1)
        return '\0';

    return ch;
}

static int editor_line_count(const TuiEditor *editor)
{
    int count;
    int len;
    int i;

    count = 1;
    len = editor_length(editor);

    for (i = 0; i < len; ++i) {
        if (editor_at(editor, i) == '\n')
            ++count;
    }

    return count;
}

static int editor_max_line_length(const TuiEditor *editor)
{
    int best;
    int cur;
    int len;
    int i;

    best = 0;
    cur = 0;
    len = editor_length(editor);

    for (i = 0; i < len; ++i) {
        if (editor_at(editor, i) == '\n') {
            cur = 0;
            continue;
        }

        ++cur;

        if (cur > best)
            best = cur;
    }

    return best;
}

static int editor_line_start(const TuiEditor *editor, int pos)
{
    while (pos > 0 && editor_at(editor, pos - 1) != '\n')
        --pos;

    return pos;
}

static int editor_line_end(const TuiEditor *editor, int pos)
{
    int len;

    len = editor_length(editor);

    while (pos < len && editor_at(editor, pos) != '\n')
        ++pos;

    return pos;
}

static void editor_cursor_xy(const TuiEditor *editor,
                             int *x,
                             int *y)
{
    int i;

    *y = 0;

    for (i = 0; i < editor->cursor_pos; ++i) {
        if (editor_at(editor, i) == '\n')
            ++*y;
    }

    *x = editor->cursor_pos -
         editor_line_start(editor, editor->cursor_pos);
}

/* Offset for a line/column; clamped to the line and the document. */
static int editor_pos_from_xy(const TuiEditor *editor,
                              int line,
                              int col)
{
    int pos;
    int end;
    int len;

    pos = 0;
    len = editor_length(editor);

    while (line > 0) {
        end = editor_line_end(editor, pos);

        if (end >= len)
            return end;

        pos = end + 1;
        --line;
    }

    end = editor_line_end(editor, pos);

    if (col < 0)
        col = 0;

    if (col > end - pos)
        col = end - pos;

    return pos + col;
}

/*
 * Decides which scroll bars are needed. Showing one reduces the
 * room of the other, so iterate; both flags only ever turn on.
 * The cursor cell counts as horizontal extent while editing.
 */
static void editor_layout(const TuiEditor *editor,
                          int *show_v,
                          int *show_h,
                          int *content_w,
                          int *content_h)
{
    int lines;
    int extent;
    int need_v;
    int need_h;
    int cx;
    int cy;
    int i;

    lines = editor_line_count(editor);
    extent = editor_max_line_length(editor);

    if (!editor->readonly) {
        editor_cursor_xy(editor, &cx, &cy);

        if (cx + 1 > extent)
            extent = cx + 1;
    }

    *show_v = 0;
    *show_h = 0;

    for (i = 0; i < 3; ++i) {
        *content_w = tui_max(0, editor->control.width - *show_v);
        *content_h = tui_max(0, editor->control.height - *show_h);

        need_v = lines > *content_h && editor->control.width >= 2;
        need_h = extent > *content_w && editor->control.height >= 2;

        if (need_v == *show_v && need_h == *show_h)
            break;

        *show_v = *show_v || need_v;
        *show_h = *show_h || need_h;
    }

    *content_w = tui_max(0, editor->control.width - *show_v);
    *content_h = tui_max(0, editor->control.height - *show_h);
}

/*
 * Single place that derives viewport and scroll bar state from the
 * text. With 'follow' (or after a resize) the cursor is kept visible.
 */
static void editor_sync(TuiEditor *editor, int follow)
{
    int show_v;
    int show_h;
    int cw;
    int ch;
    int cx;
    int cy;
    int lines;
    int extent;
    int max_top;
    int max_left;

    /* The model may have been changed behind our back. */
    if (editor->cursor_pos > editor_length(editor))
        editor->cursor_pos = editor_length(editor);

    editor_layout(editor, &show_v, &show_h, &cw, &ch);

    lines = editor_line_count(editor);
    extent = editor_max_line_length(editor);
    editor_cursor_xy(editor, &cx, &cy);

    if (!editor->readonly && cx + 1 > extent)
        extent = cx + 1;

    if (editor->control.width != editor->last_width ||
        editor->control.height != editor->last_height) {
        follow = 1;
        editor->last_width = editor->control.width;
        editor->last_height = editor->control.height;
    }

    if (follow) {
        if (ch > 0) {
            if (cy < editor->top_line)
                editor->top_line = cy;

            if (cy >= editor->top_line + ch)
                editor->top_line = cy - ch + 1;
        }

        if (cw > 0) {
            if (cx < editor->left_col)
                editor->left_col = cx;

            if (cx >= editor->left_col + cw)
                editor->left_col = cx - cw + 1;
        }
    }

    max_top = tui_max(0, lines - ch);
    max_left = tui_max(0, extent - cw);

    editor->top_line = tui_max(0, tui_min(max_top, editor->top_line));
    editor->left_col = tui_max(0, tui_min(max_left, editor->left_col));

    if (show_v) {
        editor->vscroll.control.flags |= TUI_VISIBLE;
        editor->vscroll.control.x = editor->control.width - 1;
        editor->vscroll.control.y = 0;
        editor->vscroll.control.width = 1;
        editor->vscroll.control.height = ch;
        tui_scrollbar_set_range(&editor->vscroll, 0, lines);
        tui_scrollbar_set_page(&editor->vscroll, ch);
        tui_scrollbar_set_value(&editor->vscroll, editor->top_line);
    } else {
        editor->vscroll.control.flags &= ~TUI_VISIBLE;
    }

    if (show_h) {
        editor->hscroll.control.flags |= TUI_VISIBLE;
        editor->hscroll.control.x = 0;
        editor->hscroll.control.y = editor->control.height - 1;
        editor->hscroll.control.width = cw;
        editor->hscroll.control.height = 1;
        tui_scrollbar_set_range(&editor->hscroll, 0, extent);
        tui_scrollbar_set_page(&editor->hscroll, cw);
        tui_scrollbar_set_value(&editor->hscroll, editor->left_col);
    } else {
        editor->hscroll.control.flags &= ~TUI_VISIBLE;
    }
}

/* Internal scroll bars share one adapter that updates the viewport. */
static void editor_scroll_draw(TuiControl *control, TuiDraw *draw)
{
    tui_scrollbar_class.draw(control, draw);
}

static int editor_scroll_event(TuiControl *control, TuiEvent *event)
{
    TuiEditor *editor;
    int handled;

    handled = tui_scrollbar_class.event(control, event);
    editor = (TuiEditor *)control->parent;

    if (editor != 0) {
        if (control == &editor->vscroll.control)
            editor->top_line = tui_scrollbar_get_value(&editor->vscroll);
        else
            editor->left_col = tui_scrollbar_get_value(&editor->hscroll);

        /* The viewport moved: the text has to be drawn again. */
        if (handled)
            tui_invalidate(&editor->control);
    }

    return handled;
}

static void editor_scroll_detach(TuiControl *control)
{
    tui_scrollbar_class.detach(control);
}

static const TuiClass editor_scroll_class = {
    editor_scroll_draw,
    editor_scroll_event,
    editor_scroll_detach,
    TUI_CLASS_OPAQUE
};

static void editor_init_scrollbar(TuiEditor *editor,
                                  TuiScrollBar *sb,
                                  int orientation)
{
    tui_scrollbar_init(sb, 0, 0, 1, orientation, TUI_CMD_NONE);
    sb->control.cls = &editor_scroll_class;
    sb->control.flags = TUI_ENABLED;
    tui_add(&editor->control, &sb->control);
}

void tui_editor_init(TuiEditor *editor,
                     int x,
                     int y,
                     int width,
                     int height,
                     TuiTextModel *model)
{
    tui_control_init(&editor->control,
                     &editor_class,
                     x, y,
                     width, height,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    editor->model = model;
    editor->cursor_pos = 0;
    editor->top_line = 0;
    editor->left_col = 0;
    editor->command = TUI_CMD_NONE;
    editor->modified = 0;
    editor->readonly = 0;
    editor->last_width = width;
    editor->last_height = height;

    editor_init_scrollbar(editor, &editor->vscroll, TUI_VERTICAL);
    editor_init_scrollbar(editor, &editor->hscroll, TUI_HORIZONTAL);

    if (editor->model != 0)
        editor_sync(editor, 0);
}

void tui_editor_set_command(TuiEditor *editor, int command)
{
    if (editor != 0)
        editor->command = command;
}

void tui_editor_get_position(const TuiEditor *editor,
                             TuiEditorPosition *position)
{
    int x;
    int y;

    if (position == 0)
        return;

    position->line = 0;
    position->column = 0;
    position->offset = 0;

    if (editor == 0 || editor->model == 0)
        return;

    editor_cursor_xy(editor, &x, &y);
    position->line = y;
    position->column = x;
    position->offset = editor->cursor_pos;
}

int tui_editor_is_modified(const TuiEditor *editor)
{
    return editor != 0 && editor->modified;
}

void tui_editor_set_modified(TuiEditor *editor, int modified)
{
    if (editor != 0)
        editor->modified = modified != 0;
}

void tui_editor_set_readonly(TuiEditor *editor, int readonly)
{
    if (editor == 0)
        return;

    editor->readonly = readonly != 0;
    tui_invalidate(&editor->control);

    if (editor->model != 0)
        editor_sync(editor, 0);
}

/* Each edit returns 1 only if the model really changed. */
static int editor_insert_char(TuiEditor *editor, int ch)
{
    char c;

    c = (char)ch;

    if (tui_text_model_insert(editor->model,
                              editor->cursor_pos,
                              &c, 1) != 1)
        return 0;

    ++editor->cursor_pos;
    editor->modified = 1;

    return 1;
}

static int editor_backspace(TuiEditor *editor)
{
    if (editor->cursor_pos <= 0 ||
        tui_text_model_delete(editor->model,
                              editor->cursor_pos - 1, 1) != 1)
        return 0;

    --editor->cursor_pos;
    editor->modified = 1;

    return 1;
}

static int editor_delete(TuiEditor *editor)
{
    if (tui_text_model_delete(editor->model,
                              editor->cursor_pos, 1) != 1)
        return 0;

    editor->modified = 1;

    return 1;
}

static void editor_move_vertical(TuiEditor *editor, int lines)
{
    int cx;
    int cy;
    int count;

    editor_cursor_xy(editor, &cx, &cy);
    count = editor_line_count(editor);

    cy += lines;

    if (cy < 0)
        cy = 0;

    if (cy > count - 1)
        cy = count - 1;

    editor->cursor_pos = editor_pos_from_xy(editor, cy, cx);
}

static void editor_move_page(TuiEditor *editor, int direction)
{
    int show_v;
    int show_h;
    int cw;
    int ch;

    editor_layout(editor, &show_v, &show_h, &cw, &ch);

    if (ch < 1)
        ch = 1;

    editor->top_line += direction * ch;
    editor_move_vertical(editor, direction * ch);
}

/*
 * Editing keys are consumed but ignored when read-only.
 * *changed is set when the text was modified.
 */
static int editor_edit_key(TuiEditor *editor, int key, int *changed)
{
    int editing;

    editing = key == TUI_KEY_ENTER ||
              key == TUI_KEY_BACKSPACE ||
              key == TUI_KEY_DELETE ||
              tui_console_printable(key);

    if (!editing || editor->readonly)
        return editing;

    if (key == TUI_KEY_ENTER)
        *changed = editor_insert_char(editor, '\n');
    else if (key == TUI_KEY_BACKSPACE)
        *changed = editor_backspace(editor);
    else if (key == TUI_KEY_DELETE)
        *changed = editor_delete(editor);
    else
        *changed = editor_insert_char(editor, key);

    return 1;
}

static int editor_nav_key(TuiEditor *editor, int key)
{
    switch (key) {
    case TUI_KEY_LEFT:
        if (editor->cursor_pos > 0)
            --editor->cursor_pos;
        return 1;

    case TUI_KEY_RIGHT:
        if (editor->cursor_pos < editor_length(editor))
            ++editor->cursor_pos;
        return 1;

    case TUI_KEY_UP:
        editor_move_vertical(editor, -1);
        return 1;

    case TUI_KEY_DOWN:
        editor_move_vertical(editor, 1);
        return 1;

    case TUI_KEY_HOME:
        editor->cursor_pos =
            editor_line_start(editor, editor->cursor_pos);
        return 1;

    case TUI_KEY_END:
        editor->cursor_pos =
            editor_line_end(editor, editor->cursor_pos);
        return 1;

    case TUI_KEY_PAGEUP:
        editor_move_page(editor, -1);
        return 1;

    case TUI_KEY_PAGEDOWN:
        editor_move_page(editor, 1);
        return 1;
    }

    return 0;
}

static void editor_mouse(TuiEditor *editor, TuiEvent *event)
{
    int show_v;
    int show_h;
    int cw;
    int ch;
    int x;
    int y;

    editor_sync(editor, 0);
    editor_layout(editor, &show_v, &show_h, &cw, &ch);
    tui_control_screen_to_local(&editor->control,
                                event->mouse_x,
                                event->mouse_y,
                                &x, &y);

    /* Scroll bar strips and the corner never move the cursor. */
    if (x < 0 || y < 0 || x >= cw || y >= ch)
        return;

    editor->cursor_pos = editor_pos_from_xy(editor,
                                            editor->top_line + y,
                                            editor->left_col + x);
    editor_sync(editor, 1);
}

static int editor_handle(TuiControl *control, TuiEvent *event);

/* Whatever the control handled may have changed what it shows. */
static int editor_event(TuiControl *control, TuiEvent *event)
{
    int handled;

    handled = editor_handle(control, event);

    if (handled)
        tui_invalidate(control);

    return handled;
}

static int editor_handle(TuiControl *control, TuiEvent *event)
{
    TuiEditor *editor;
    int old_pos;
    int changed;
    int handled;

    editor = (TuiEditor *)control;

    if (editor->model == 0)
        return 0;

    old_pos = editor->cursor_pos;
    changed = 0;

    if (event->type == TUI_EV_MOUSE) {
        if (!((event->mouse_action == TUI_MOUSE_DOWN ||
               event->mouse_action == TUI_MOUSE_DOUBLE) &&
              (event->mouse_buttons & TUI_MOUSE_LEFT)))
            return 0;

        editor_mouse(editor, event);
        handled = 1;
    } else if (event->type == TUI_EV_KEY) {
        handled = editor_nav_key(editor, event->key);

        if (!handled)
            handled = editor_edit_key(editor, event->key, &changed);

        if (!handled)
            return 0;

        editor_sync(editor, 1);
    } else {
        return 0;
    }

    /* One command per user action, if anything observable changed. */
    if (editor->command != TUI_CMD_NONE &&
        (changed || editor->cursor_pos != old_pos)) {
        event->type = TUI_EV_COMMAND;
        event->command = editor->command;
        event->source = control;
    }

    return handled;
}

static void editor_draw(TuiControl *control, TuiDraw *draw)
{
    TuiEditor *editor;
    int show_v;
    int show_h;
    int cw;
    int ch;
    int attr;
    int row;
    int col;
    int pos;
    int len;
    int cx;
    int cy;
    int end;
    int index;

    editor = (TuiEditor *)control;
    attr = tui_control_attr(control);

    if (editor->model == 0) {
        tui_fill(draw, 0, 0, control->width, control->height, ' ', attr);
        return;
    }

    editor_sync(editor, 0);
    editor_layout(editor, &show_v, &show_h, &cw, &ch);

    tui_fill(draw, 0, 0, cw, ch, ' ', attr);

    if (show_v && show_h) {
        tui_fill(draw, cw, ch, 1, 1, ' ',
                 TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY));
    }

    len = editor_length(editor);

    /* Find the first visible line. */
    pos = 0;

    for (row = 0; row < editor->top_line && pos >= 0; ++row) {
        pos = editor_line_end(editor, pos);
        pos = pos < len ? pos + 1 : -1;
    }

    for (row = 0; row < ch && pos >= 0; ++row) {
        end = editor_line_end(editor, pos);

        for (col = 0; col < cw; ++col) {
            index = pos + editor->left_col + col;

            if (index >= end)
                break;

            tui_putc(draw, col, row,
                     (unsigned char)editor_at(editor, index),
                     attr);
        }

        pos = end < len ? end + 1 : -1;
    }

    if (!editor->readonly && tui_control_has_focus(control)) {
        editor_cursor_xy(editor, &cx, &cy);
        cx -= editor->left_col;
        cy -= editor->top_line;

        if (cx >= 0 && cx < cw && cy >= 0 && cy < ch)
            tui_draw_cursor(draw, cx, cy);
    }
}
