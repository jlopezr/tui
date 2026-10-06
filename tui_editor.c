#include "tui_internal.h"

static void editor_draw(TuiControl *control, TuiDraw *draw);
static int editor_event(TuiControl *control, TuiEvent *event);
static void editor_focus_changed(TuiControl *control);

static const TuiClass editor_class = {
    editor_draw,
    editor_event,
    0,
    TUI_CLASS_OPAQUE,
    editor_focus_changed
};

/*
 * ------------------------------------------------------------
 * Editor
 *
 * Everything is derived from the model through its public
 * operations only; lines are found by walking the characters.
 * Nothing is cached on purpose.
 *
 * The text is read EDITOR_CHUNK characters at a time, never one
 * per call: each call goes through the model's class and costs
 * far more than looking at the character, and one key press used
 * to make thousands of them (a handful of passes over the text,
 * at ~100 instructions a character on the MiniCPU).
 * ------------------------------------------------------------
 */

#define EDITOR_CHUNK 32

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
    char buf[EDITOR_CHUNK];
    int count;
    int pos;
    int len;
    int n;
    int i;

    count = 1;
    pos = 0;
    len = editor_length(editor);

    while (pos < len) {
        n = tui_text_model_read(editor->model, pos, buf, EDITOR_CHUNK);

        if (n <= 0)
            break;

        for (i = 0; i < n; ++i) {
            if (buf[i] == '\n')
                ++count;
        }

        pos += n;
    }

    return count;
}

/* Number of lines and length of the longest one, in a single pass over the text. */
static void editor_measure(const TuiEditor *editor,
                           int *lines,
                           int *extent)
{
    char buf[EDITOR_CHUNK];
    int cur;
    int pos;
    int len;
    int n;
    int i;

    *lines = 1;
    *extent = 0;
    cur = 0;
    pos = 0;
    len = editor_length(editor);

    while (pos < len) {
        n = tui_text_model_read(editor->model, pos, buf, EDITOR_CHUNK);

        if (n <= 0)
            break;

        for (i = 0; i < n; ++i) {
            if (buf[i] == '\n') {
                ++*lines;
                cur = 0;
                continue;
            }

            ++cur;

            if (cur > *extent)
                *extent = cur;
        }

        pos += n;
    }
}

static int editor_line_start(const TuiEditor *editor, int pos)
{
    char buf[EDITOR_CHUNK];
    int start;
    int n;
    int i;

    while (pos > 0) {
        start = pos > EDITOR_CHUNK ? pos - EDITOR_CHUNK : 0;
        n = tui_text_model_read(editor->model, start, buf, pos - start);

        if (n != pos - start) {
            /* A short read (the position is past the text): one at a time. */
            while (pos > 0 && editor_at(editor, pos - 1) != '\n')
                --pos;

            return pos;
        }

        for (i = n - 1; i >= 0; --i) {
            if (buf[i] == '\n')
                return start + i + 1;
        }

        pos = start;
    }

    return 0;
}

static int editor_line_end(const TuiEditor *editor, int pos)
{
    char buf[EDITOR_CHUNK];
    int len;
    int n;
    int i;

    len = editor_length(editor);

    while (pos < len) {
        n = tui_text_model_read(editor->model, pos, buf, EDITOR_CHUNK);

        if (n <= 0)
            return len;

        for (i = 0; i < n; ++i) {
            if (buf[i] == '\n')
                return pos + i;
        }

        pos += n;
    }

    return pos;
}

/* Line and column of the cursor, with a single walk from the start of the text. */
static void editor_cursor_xy(const TuiEditor *editor,
                             int *x,
                             int *y)
{
    char buf[EDITOR_CHUNK];
    int line_start;
    int pos;
    int want;
    int n;
    int i;

    *y = 0;
    line_start = 0;
    pos = 0;

    while (pos < editor->cursor_pos) {
        want = editor->cursor_pos - pos;

        if (want > EDITOR_CHUNK)
            want = EDITOR_CHUNK;

        n = tui_text_model_read(editor->model, pos, buf, want);

        if (n <= 0)
            break;

        for (i = 0; i < n; ++i) {
            if (buf[i] == '\n') {
                ++*y;
                line_start = pos + i + 1;
            }
        }

        pos += n;
    }

    *x = editor->cursor_pos - line_start;
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
static void editor_layout_from(const TuiEditor *editor,
                               int lines,
                               int extent,
                               int *show_v,
                               int *show_h,
                               int *content_w,
                               int *content_h)
{
    int need_v;
    int need_h;
    int i;

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

/* The same, finding the lines and the extent (cursor cell included) itself. */
static void editor_layout(const TuiEditor *editor,
                          int *show_v,
                          int *show_h,
                          int *content_w,
                          int *content_h)
{
    int lines;
    int extent;
    int cx;
    int cy;

    editor_measure(editor, &lines, &extent);

    if (!editor->readonly) {
        editor_cursor_xy(editor, &cx, &cy);

        if (cx + 1 > extent)
            extent = cx + 1;
    }

    editor_layout_from(editor, lines, extent,
                       show_v, show_h, content_w, content_h);
}

/* What editor_sync() found out, for whoever needs it next (drawing, the mouse). */
typedef struct EditorView {
    int show_v;
    int show_h;
    int cw;
    int ch;
    int cx;
    int cy;
    int lines;
    int extent;         /* the longest line, or the cursor cell if it is further */
} EditorView;

/*
 * Single place that derives viewport and scroll bar state from the
 * text. With 'follow' (or after a resize) the cursor is kept visible.
 * The text is walked once for the layout and once up to the cursor, and
 * the result is left in 'view' so that nobody has to walk it again.
 */
static void editor_sync_view(TuiEditor *editor, int follow, EditorView *view)
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

    editor_measure(editor, &lines, &extent);
    editor_cursor_xy(editor, &cx, &cy);

    if (!editor->readonly && cx + 1 > extent)
        extent = cx + 1;

    editor_layout_from(editor, lines, extent, &show_v, &show_h, &cw, &ch);

    view->show_v = show_v;
    view->show_h = show_h;
    view->cw = cw;
    view->ch = ch;
    view->cx = cx;
    view->cy = cy;
    view->lines = lines;
    view->extent = extent;

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

static void editor_sync(TuiEditor *editor, int follow)
{
    EditorView view;

    editor_sync_view(editor, follow, &view);
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
    EditorView view;
    int cw;
    int ch;
    int x;
    int y;

    editor_sync_view(editor, 0, &view);
    cw = view.cw;
    ch = view.ch;
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

/* What an event may change on screen, as it was before the event. */
typedef struct EditorBefore {
    int top_line;
    int left_col;
    int show_v;
    int show_h;
    int length;
    int lines;
    int extent;
    int cx;
    int cy;
} EditorBefore;

static void editor_snapshot(const TuiEditor *editor, EditorBefore *before)
{
    before->top_line = editor->top_line;
    before->left_col = editor->left_col;
    before->show_v = (editor->vscroll.control.flags & TUI_VISIBLE) != 0;
    before->show_h = (editor->hscroll.control.flags & TUI_VISIBLE) != 0;
    before->length = editor_length(editor);

    editor_measure(editor, &before->lines, &before->extent);
    editor_cursor_xy(editor, &before->cx, &before->cy);

    if (!editor->readonly && before->cx + 1 > before->extent)
        before->extent = before->cx + 1;
}

/*
 * Reports to the desktop what the event changed, and no more: the editor used to
 * invalidate itself whole after anything, which is two thousand cells to move the
 * cursor. It is the handler, not the draw function, that knows what happened.
 *
 *   the viewport moved (it scrolled, a scroll bar came or went)   all of it
 *   lines were added or removed                                   from the first
 *                                                                 changed row down
 *   a character was typed or deleted on a line                   that row, from the
 *                                                                 column that changed
 *   only the cursor moved                                         the cell it is on
 *
 * The scroll bars are children: they are told only when their thumb changes (their
 * setters do not invalidate).
 */
static void editor_invalidate_changes(TuiEditor *editor,
                                      const EditorBefore *before)
{
    EditorView view;
    TuiControl *control;
    int cx;
    int cy;
    int col;
    int row;
    int last;

    control = &editor->control;
    editor_sync_view(editor, 0, &view);

    if (editor->top_line != before->top_line ||
        editor->left_col != before->left_col ||
        view.show_v != before->show_v ||
        view.show_h != before->show_h) {
        tui_invalidate(control);
        return;
    }

    cx = view.cx - editor->left_col;
    cy = view.cy - editor->top_line;

    if (cx < 0 || cx >= view.cw || cy < 0 || cy >= view.ch) {
        tui_invalidate(control);
        return;
    }

    if (view.show_v && view.lines != before->lines)
        tui_invalidate(&editor->vscroll.control);

    if (view.show_h && view.extent != before->extent)
        tui_invalidate(&editor->hscroll.control);

    if (editor_length(editor) == before->length) {
        /* Nothing was typed: either the cursor moved, or nothing happened. */
        if (view.cx == before->cx && view.cy == before->cy)
            tui_event_done(control);
        else
            tui_invalidate_rect(control, cx, cy, 1, 1);

        return;
    }

    if (view.lines != before->lines) {
        /* Down to the last line there was, or is: below it is background. */
        row = tui_min(before->cy, view.cy) - editor->top_line;
        row = tui_max(0, row);
        last = tui_max(before->lines, view.lines) - editor->top_line;
        last = tui_min(view.ch, last);
        tui_invalidate_rect(control, 0, row, view.cw, last - row);
        return;
    }

    col = tui_min(before->cx, view.cx) - editor->left_col;
    col = tui_max(0, col);
    tui_invalidate_rect(control, col, cy, view.cw - col, 1);
}

/*
 * The focus moved onto or off the editor. The only thing that looks different is
 * the cursor, which editor_draw() asks for while the editor has the focus, so the
 * cell it is on is all there is to draw (the desktop has already hidden the cursor,
 * and shows it again when that cell is drawn). Nothing here may change the
 * viewport: the control may not have been laid out yet, so the size is only used to
 * check that the cursor is in view, and if it is not the whole editor is drawn.
 */
static void editor_focus_changed(TuiControl *control)
{
    TuiEditor *editor;
    int cw;
    int ch;
    int cx;
    int cy;

    editor = (TuiEditor *)control;

    if (editor->model == 0 || editor->readonly) {
        /* No cursor to show or hide. */
        tui_event_done(control);
        return;
    }

    cw = control->width - ((editor->vscroll.control.flags & TUI_VISIBLE) != 0);
    ch = control->height - ((editor->hscroll.control.flags & TUI_VISIBLE) != 0);

    editor_cursor_xy(editor, &cx, &cy);
    cx -= editor->left_col;
    cy -= editor->top_line;

    if (cx >= 0 && cx < cw && cy >= 0 && cy < ch)
        tui_invalidate_rect(control, cx, cy, 1, 1);
    else
        tui_invalidate(control);
}

/* Whatever the control handled may have changed what it shows. */
static int editor_event(TuiControl *control, TuiEvent *event)
{
    TuiEditor *editor;
    EditorBefore before;
    int handled;

    editor = (TuiEditor *)control;

    if (editor->model == 0)
        return editor_handle(control, event);

    editor_snapshot(editor, &before);

    handled = editor_handle(control, event);

    if (handled)
        editor_invalidate_changes(editor, &before);

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
    EditorView view;
    char buf[EDITOR_CHUNK];
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
    int n;
    int i;
    int row_from;
    int row_to;
    int col_from;
    int col_to;

    editor = (TuiEditor *)control;
    attr = tui_control_attr(control);

    if (editor->model == 0) {
        tui_fill(draw, 0, 0, control->width, control->height, ' ', attr);
        return;
    }

    editor_sync_view(editor, 0, &view);
    cw = view.cw;
    ch = view.ch;

    tui_fill(draw, 0, 0, cw, ch, ' ', attr);

    if (view.show_v && view.show_h) {
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

    /*
     * Only what the clip lets through is worth reading: rows above it are walked
     * past (their length is needed to find the next one), rows below it are not
     * looked at, and in the rows that are drawn only the columns it covers are read.
     */
    row_from = draw->y1 - draw->oy;
    row_to = draw->y2 - draw->oy;
    col_from = tui_max(0, draw->x1 - draw->ox);
    col_to = tui_min(cw, draw->x2 - draw->ox);

    for (row = 0; row < ch && row < row_to && pos >= 0; ++row) {
        end = editor_line_end(editor, pos);

        if (row >= row_from) {
            /* The visible part of the line, a chunk of the model at a time. */
            col = col_from;
            index = pos + editor->left_col + col;

            while (col < col_to && index < end) {
                n = tui_min(tui_min(col_to - col, end - index), EDITOR_CHUNK);
                n = tui_text_model_read(editor->model, index, buf, n);

                if (n <= 0)
                    break;

                for (i = 0; i < n; ++i)
                    tui_putc(draw, col + i, row, (unsigned char)buf[i], attr);

                col += n;
                index += n;
            }
        }

        pos = end < len ? end + 1 : -1;
    }

    if (!editor->readonly && tui_control_has_focus(control)) {
        cx = view.cx - editor->left_col;
        cy = view.cy - editor->top_line;

        if (cx >= 0 && cx < cw && cy >= 0 && cy < ch)
            tui_draw_cursor(draw, cx, cy);
    }
}
