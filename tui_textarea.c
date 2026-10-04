#include "tui_internal.h"

static void textarea_draw(TuiControl *control, TuiDraw *draw);
static int textarea_event(TuiControl *control, TuiEvent *event);

static const TuiClass textarea_class = {
    textarea_draw,
    textarea_event,
    0,
    TUI_CLASS_OPAQUE
};

/*
 * ------------------------------------------------------------
 * Text area
 *
 * The document is just the application's C string; lines are
 * located by walking it. Nothing is cached on purpose.
 * ------------------------------------------------------------
 */

static int textarea_length(const TuiTextArea *area)
{
    int len;

    len = 0;

    if (area->text != 0) {
        while (area->text[len] != '\0')
            ++len;
    }

    return len;
}

static int textarea_line_count(const TuiTextArea *area)
{
    int count;
    int i;

    count = 1;

    if (area->text == 0)
        return count;

    for (i = 0; area->text[i] != '\0'; ++i) {
        if (area->text[i] == '\n')
            ++count;
    }

    return count;
}

static int textarea_max_line_length(const TuiTextArea *area)
{
    int best;
    int cur;
    int i;

    best = 0;
    cur = 0;

    if (area->text == 0)
        return 0;

    for (i = 0; area->text[i] != '\0'; ++i) {
        if (area->text[i] == '\n') {
            cur = 0;
            continue;
        }

        ++cur;

        if (cur > best)
            best = cur;
    }

    return best;
}

static int textarea_line_start(const TuiTextArea *area, int pos)
{
    while (pos > 0 && area->text[pos - 1] != '\n')
        --pos;

    return pos;
}

static int textarea_line_end(const TuiTextArea *area, int pos)
{
    while (area->text[pos] != '\0' && area->text[pos] != '\n')
        ++pos;

    return pos;
}

static void textarea_cursor_xy(const TuiTextArea *area,
                               int *x,
                               int *y)
{
    int i;

    *y = 0;

    for (i = 0; i < area->cursor_pos; ++i) {
        if (area->text[i] == '\n')
            ++*y;
    }

    *x = area->cursor_pos - textarea_line_start(area, area->cursor_pos);
}

/* Offset for a line/column; clamped to the line and the document. */
static int textarea_pos_from_xy(const TuiTextArea *area,
                                int line,
                                int col)
{
    int pos;
    int end;

    pos = 0;

    while (line > 0) {
        end = textarea_line_end(area, pos);

        if (area->text[end] == '\0')
            return end;

        pos = end + 1;
        --line;
    }

    end = textarea_line_end(area, pos);

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
static void textarea_layout(const TuiTextArea *area,
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

    lines = textarea_line_count(area);
    extent = textarea_max_line_length(area);

    if (!area->readonly) {
        textarea_cursor_xy(area, &cx, &cy);

        if (cx + 1 > extent)
            extent = cx + 1;
    }

    *show_v = 0;
    *show_h = 0;

    for (i = 0; i < 3; ++i) {
        *content_w = tui_max(0, area->control.width - *show_v);
        *content_h = tui_max(0, area->control.height - *show_h);

        need_v = lines > *content_h && area->control.width >= 2;
        need_h = extent > *content_w && area->control.height >= 2;

        if (need_v == *show_v && need_h == *show_h)
            break;

        *show_v = *show_v || need_v;
        *show_h = *show_h || need_h;
    }

    *content_w = tui_max(0, area->control.width - *show_v);
    *content_h = tui_max(0, area->control.height - *show_h);
}

/*
 * Single place that derives viewport and scroll bar state from the
 * text. With 'follow' (or after a resize) the cursor is kept visible.
 */
static void textarea_sync(TuiTextArea *area, int follow)
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

    textarea_layout(area, &show_v, &show_h, &cw, &ch);

    lines = textarea_line_count(area);
    extent = textarea_max_line_length(area);
    textarea_cursor_xy(area, &cx, &cy);

    if (!area->readonly && cx + 1 > extent)
        extent = cx + 1;

    if (area->control.width != area->last_width ||
        area->control.height != area->last_height) {
        follow = 1;
        area->last_width = area->control.width;
        area->last_height = area->control.height;
    }

    if (follow) {
        if (ch > 0) {
            if (cy < area->top_line)
                area->top_line = cy;

            if (cy >= area->top_line + ch)
                area->top_line = cy - ch + 1;
        }

        if (cw > 0) {
            if (cx < area->left_col)
                area->left_col = cx;

            if (cx >= area->left_col + cw)
                area->left_col = cx - cw + 1;
        }
    }

    max_top = tui_max(0, lines - ch);
    max_left = tui_max(0, extent - cw);

    area->top_line = tui_max(0, tui_min(max_top, area->top_line));
    area->left_col = tui_max(0, tui_min(max_left, area->left_col));

    if (show_v) {
        area->vscroll.control.flags |= TUI_VISIBLE;
        area->vscroll.control.x = area->control.width - 1;
        area->vscroll.control.y = 0;
        area->vscroll.control.width = 1;
        area->vscroll.control.height = ch;
        tui_scrollbar_set_range(&area->vscroll, 0, lines);
        tui_scrollbar_set_page(&area->vscroll, ch);
        tui_scrollbar_set_value(&area->vscroll, area->top_line);
    } else {
        area->vscroll.control.flags &= ~TUI_VISIBLE;
    }

    if (show_h) {
        area->hscroll.control.flags |= TUI_VISIBLE;
        area->hscroll.control.x = 0;
        area->hscroll.control.y = area->control.height - 1;
        area->hscroll.control.width = cw;
        area->hscroll.control.height = 1;
        tui_scrollbar_set_range(&area->hscroll, 0, extent);
        tui_scrollbar_set_page(&area->hscroll, cw);
        tui_scrollbar_set_value(&area->hscroll, area->left_col);
    } else {
        area->hscroll.control.flags &= ~TUI_VISIBLE;
    }
}

/* Internal scroll bars share one adapter that updates the viewport. */
static void textarea_scroll_draw(TuiControl *control, TuiDraw *draw)
{
    tui_scrollbar_class.draw(control, draw);
}

static int textarea_scroll_event(TuiControl *control, TuiEvent *event)
{
    TuiTextArea *area;
    int handled;

    handled = tui_scrollbar_class.event(control, event);
    area = (TuiTextArea *)control->parent;

    if (area != 0) {
        if (control == &area->vscroll.control)
            area->top_line = tui_scrollbar_get_value(&area->vscroll);
        else
            area->left_col = tui_scrollbar_get_value(&area->hscroll);

        /* The viewport moved: the text has to be drawn again. */
        if (handled)
            tui_invalidate(&area->control);
    }

    return handled;
}

static void textarea_scroll_detach(TuiControl *control)
{
    tui_scrollbar_class.detach(control);
}

static const TuiClass textarea_scroll_class = {
    textarea_scroll_draw,
    textarea_scroll_event,
    textarea_scroll_detach,
    TUI_CLASS_OPAQUE
};

static void textarea_init_scrollbar(TuiTextArea *area,
                                    TuiScrollBar *sb,
                                    int orientation)
{
    tui_scrollbar_init(sb, 0, 0, 1, orientation, TUI_CMD_NONE);
    sb->control.cls = &textarea_scroll_class;
    sb->control.flags = TUI_ENABLED;
    tui_add(&area->control, &sb->control);
}

void tui_textarea_init(TuiTextArea *area,
                       int x,
                       int y,
                       int width,
                       int height,
                       char *buffer,
                       int capacity)
{
    tui_control_init(&area->control,
                     &textarea_class,
                     x, y,
                     width, height,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    area->text = capacity > 0 ? buffer : 0;
    area->capacity = area->text != 0 ? capacity : 0;

    /* Guarantee a terminated string inside the buffer. */
    if (area->text != 0)
        area->text[area->capacity - 1] = '\0';

    area->cursor_pos = 0;
    area->top_line = 0;
    area->left_col = 0;
    area->readonly = 0;
    area->last_width = width;
    area->last_height = height;

    textarea_init_scrollbar(area, &area->vscroll, TUI_VERTICAL);
    textarea_init_scrollbar(area, &area->hscroll, TUI_HORIZONTAL);

    textarea_sync(area, 0);
}

void tui_textarea_set_text(TuiTextArea *area, const char *text)
{
    int i;

    if (area == 0 || area->text == 0)
        return;

    tui_invalidate(&area->control);

    i = 0;

    if (text != 0) {
        while (i < area->capacity - 1 && text[i] != '\0') {
            area->text[i] = text[i];
            ++i;
        }
    }

    area->text[i] = '\0';

    area->cursor_pos = 0;
    area->top_line = 0;
    area->left_col = 0;
    textarea_sync(area, 0);
}

const char *tui_textarea_get_text(const TuiTextArea *area)
{
    if (area == 0 || area->text == 0)
        return "";

    return area->text;
}

void tui_textarea_set_readonly(TuiTextArea *area, int readonly)
{
    if (area == 0)
        return;

    area->readonly = readonly != 0;
    textarea_sync(area, 0);
}

static void textarea_insert_char(TuiTextArea *area, int ch)
{
    int len;
    int i;

    len = textarea_length(area);

    if (len >= area->capacity - 1)
        return;

    /* Shift the tail, including the terminator. */
    for (i = len; i >= area->cursor_pos; --i)
        area->text[i + 1] = area->text[i];

    area->text[area->cursor_pos] = (char)ch;
    ++area->cursor_pos;
}

static void textarea_remove_at(TuiTextArea *area, int pos)
{
    int i;

    for (i = pos; area->text[i] != '\0'; ++i)
        area->text[i] = area->text[i + 1];
}

static void textarea_backspace(TuiTextArea *area)
{
    if (area->cursor_pos <= 0)
        return;

    textarea_remove_at(area, area->cursor_pos - 1);
    --area->cursor_pos;
}

static void textarea_delete(TuiTextArea *area)
{
    if (area->text[area->cursor_pos] == '\0')
        return;

    textarea_remove_at(area, area->cursor_pos);
}

static void textarea_move_vertical(TuiTextArea *area, int lines)
{
    int cx;
    int cy;
    int count;

    textarea_cursor_xy(area, &cx, &cy);
    count = textarea_line_count(area);

    cy += lines;

    if (cy < 0)
        cy = 0;

    if (cy > count - 1)
        cy = count - 1;

    area->cursor_pos = textarea_pos_from_xy(area, cy, cx);
}

static void textarea_move_page(TuiTextArea *area, int direction)
{
    int show_v;
    int show_h;
    int cw;
    int ch;

    textarea_layout(area, &show_v, &show_h, &cw, &ch);

    if (ch < 1)
        ch = 1;

    area->top_line += direction * ch;
    textarea_move_vertical(area, direction * ch);
}

/* Editing keys are consumed but ignored when read-only. */
static int textarea_edit_key(TuiTextArea *area, int key)
{
    int editing;

    editing = key == TUI_KEY_ENTER ||
              key == TUI_KEY_BACKSPACE ||
              key == TUI_KEY_DELETE ||
              tui_console_printable(key);

    if (!editing || area->readonly)
        return editing;

    if (key == TUI_KEY_ENTER)
        textarea_insert_char(area, '\n');
    else if (key == TUI_KEY_BACKSPACE)
        textarea_backspace(area);
    else if (key == TUI_KEY_DELETE)
        textarea_delete(area);
    else
        textarea_insert_char(area, key);

    return 1;
}

static int textarea_nav_key(TuiTextArea *area, int key)
{
    switch (key) {
    case TUI_KEY_LEFT:
        if (area->cursor_pos > 0)
            --area->cursor_pos;
        return 1;

    case TUI_KEY_RIGHT:
        if (area->text[area->cursor_pos] != '\0')
            ++area->cursor_pos;
        return 1;

    case TUI_KEY_UP:
        textarea_move_vertical(area, -1);
        return 1;

    case TUI_KEY_DOWN:
        textarea_move_vertical(area, 1);
        return 1;

    case TUI_KEY_HOME:
        area->cursor_pos = textarea_line_start(area, area->cursor_pos);
        return 1;

    case TUI_KEY_END:
        area->cursor_pos = textarea_line_end(area, area->cursor_pos);
        return 1;

    case TUI_KEY_PAGEUP:
        textarea_move_page(area, -1);
        return 1;

    case TUI_KEY_PAGEDOWN:
        textarea_move_page(area, 1);
        return 1;
    }

    return 0;
}

static int textarea_mouse(TuiTextArea *area, TuiEvent *event)
{
    int show_v;
    int show_h;
    int cw;
    int ch;
    int x;
    int y;

    textarea_sync(area, 0);
    textarea_layout(area, &show_v, &show_h, &cw, &ch);
    tui_control_screen_to_local(&area->control,
                                event->mouse_x,
                                event->mouse_y,
                                &x, &y);

    /* Scroll bar strips and the corner never move the cursor. */
    if (x < 0 || y < 0 || x >= cw || y >= ch)
        return 1;

    area->cursor_pos = textarea_pos_from_xy(area,
                                            area->top_line + y,
                                            area->left_col + x);
    textarea_sync(area, 1);

    return 1;
}

static int textarea_handle(TuiControl *control, TuiEvent *event);

/* Whatever the control handled may have changed what it shows. */
static int textarea_event(TuiControl *control, TuiEvent *event)
{
    int handled;

    handled = textarea_handle(control, event);

    if (handled)
        tui_invalidate(control);

    return handled;
}

static int textarea_handle(TuiControl *control, TuiEvent *event)
{
    TuiTextArea *area;
    int handled;

    area = (TuiTextArea *)control;

    if (area->text == 0)
        return 0;

    if (event->type == TUI_EV_MOUSE) {
        if ((event->mouse_action == TUI_MOUSE_DOWN ||
             event->mouse_action == TUI_MOUSE_DOUBLE) &&
            (event->mouse_buttons & TUI_MOUSE_LEFT))
            return textarea_mouse(area, event);

        return 0;
    }

    if (event->type != TUI_EV_KEY)
        return 0;

    handled = textarea_nav_key(area, event->key);

    if (!handled)
        handled = textarea_edit_key(area, event->key);

    if (handled)
        textarea_sync(area, 1);

    return handled;
}

static void textarea_draw(TuiControl *control, TuiDraw *draw)
{
    TuiTextArea *area;
    int show_v;
    int show_h;
    int cw;
    int ch;
    int attr;
    int row;
    int col;
    int pos;
    int line;
    int cx;
    int cy;

    area = (TuiTextArea *)control;
    attr = tui_control_attr(control);

    textarea_sync(area, 0);
    textarea_layout(area, &show_v, &show_h, &cw, &ch);

    tui_fill(draw, 0, 0, cw, ch, ' ', attr);

    if (show_v && show_h) {
        tui_fill(draw, cw, ch, 1, 1, ' ',
                 TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY));
    }

    if (area->text == 0)
        return;

    /* Find the first visible line. */
    pos = 0;

    for (line = 0; line < area->top_line; ++line) {
        pos = textarea_line_end(area, pos);

        if (area->text[pos] == '\0')
            break;

        ++pos;
    }

    if (line < area->top_line)
        pos = -1;

    for (row = 0; row < ch && pos >= 0; ++row) {
        int end;

        end = textarea_line_end(area, pos);

        for (col = 0; col < cw; ++col) {
            int index;

            index = pos + area->left_col + col;

            if (index >= end)
                break;

            tui_putc(draw, col, row,
                     (unsigned char)area->text[index],
                     attr);
        }

        if (area->text[end] == '\0')
            break;

        pos = end + 1;
    }

    if (!area->readonly && tui_control_has_focus(control)) {
        textarea_cursor_xy(area, &cx, &cy);
        cx -= area->left_col;
        cy -= area->top_line;

        if (cx >= 0 && cx < cw && cy >= 0 && cy < ch)
            tui_draw_cursor(draw, cx, cy);
    }
}
