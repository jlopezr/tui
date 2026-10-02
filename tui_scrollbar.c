#include "tui_internal.h"

static void scrollbar_draw(TuiControl *control, TuiDraw *draw);
static int scrollbar_event(TuiControl *control, TuiEvent *event);

const TuiClass tui_scrollbar_class = {
    scrollbar_draw,
    scrollbar_event
};

/*
 * ------------------------------------------------------------
 * Scroll bar
 * ------------------------------------------------------------
 */

/* Number of cells along the bar axis. */
static int scrollbar_length(const TuiScrollBar *sb)
{
    if (sb->orientation == TUI_HORIZONTAL)
        return sb->control.width;

    return sb->control.height;
}

/* Largest reachable value: max - page, never below min. */
static int scrollbar_max_value(const TuiScrollBar *sb)
{
    return tui_max(sb->min, sb->max - sb->page);
}

/* Keeps range, page and value consistent. */
static void scrollbar_normalize(TuiScrollBar *sb)
{
    if (sb->max < sb->min)
        sb->max = sb->min;

    if (sb->page < 0)
        sb->page = 0;

    if (sb->value < sb->min)
        sb->value = sb->min;

    if (sb->value > scrollbar_max_value(sb))
        sb->value = scrollbar_max_value(sb);
}

/* Cells between the two arrows. */
static int scrollbar_track(const TuiScrollBar *sb)
{
    int length;

    length = scrollbar_length(sb);

    return length > 2 ? length - 2 : 0;
}

static int scrollbar_thumb_size(const TuiScrollBar *sb)
{
    int track;
    int range;
    int size;

    track = scrollbar_track(sb);
    range = sb->max - sb->min;

    if (track == 0)
        return 0;

    if (range <= 0 || sb->page >= range)
        return track;

    size = (int)((long)track * sb->page / range);

    return tui_min(track, tui_max(1, size));
}

/* Thumb offset inside the track, 0 .. track - size. */
static int scrollbar_thumb_pos(const TuiScrollBar *sb)
{
    int free_cells;
    int span;

    free_cells = scrollbar_track(sb) - scrollbar_thumb_size(sb);
    span = scrollbar_max_value(sb) - sb->min;

    if (free_cells <= 0 || span <= 0)
        return 0;

    return (int)((long)(sb->value - sb->min) * free_cells / span);
}

/* Value matching a thumb offset; inverse of scrollbar_thumb_pos. */
static int scrollbar_value_from_pos(const TuiScrollBar *sb, int pos)
{
    int free_cells;
    int span;

    free_cells = scrollbar_track(sb) - scrollbar_thumb_size(sb);
    span = scrollbar_max_value(sb) - sb->min;

    if (free_cells <= 0 || span <= 0)
        return sb->min;

    pos = tui_max(0, tui_min(free_cells, pos));

    return sb->min +
           (int)(((long)pos * span + free_cells / 2) / free_cells);
}

static void scrollbar_draw(TuiControl *control, TuiDraw *draw)
{
    TuiScrollBar *sb;
    int length;
    int track;
    int start;
    int size;
    int i;
    int vertical;
    int attr;
    int thumb_attr;

    sb = (TuiScrollBar *)control;
    length = scrollbar_length(sb);
    vertical = sb->orientation == TUI_VERTICAL;

    attr = TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);

    if (tui_control_has_focus(control))
        thumb_attr = TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);
    else
        thumb_attr = attr;

    if (length <= 0)
        return;

    track = scrollbar_track(sb);
    start = scrollbar_thumb_pos(sb);
    size = scrollbar_thumb_size(sb);

    for (i = 0; i < length; ++i) {
        int ch;
        int a;

        a = attr;

        if (i == 0) {
            ch = vertical ? TUI_CH_UP_TRIANGLE : TUI_CH_LEFT_TRIANGLE;
        } else if (i == length - 1) {
            ch = vertical ? TUI_CH_DOWN_TRIANGLE
                          : TUI_CH_RIGHT_TRIANGLE;
        } else if (i - 1 >= start && i - 1 < start + size) {
            ch = TUI_CH_SCROLL_THUMB;
            a = thumb_attr;
        } else {
            ch = TUI_CH_SCROLL_TRACK;
        }

        if (vertical)
            tui_putc(draw, 0, i, ch, a);
        else
            tui_putc(draw, i, 0, ch, a);
    }

    (void)track;
}

/* Applies a user-driven change; returns 1 when the value moved. */
static int scrollbar_user_set(TuiScrollBar *sb, int value)
{
    int old;

    old = sb->value;
    sb->value = value;
    scrollbar_normalize(sb);

    return sb->value != old;
}

/* Turns a user change into a command; the event is always consumed. */
static int scrollbar_finish(TuiScrollBar *sb,
                            TuiEvent *event,
                            int changed)
{
    if (changed && sb->command != TUI_CMD_NONE) {
        event->type = TUI_EV_COMMAND;
        event->command = sb->command;
        event->source = &sb->control;
    }

    return 1;
}

static int scrollbar_step(const TuiScrollBar *sb)
{
    return tui_max(1, sb->page);
}

static int scrollbar_key(TuiScrollBar *sb, TuiEvent *event)
{
    int prev;
    int next;
    int vertical;

    vertical = sb->orientation == TUI_VERTICAL;
    prev = vertical ? TUI_KEY_UP : TUI_KEY_LEFT;
    next = vertical ? TUI_KEY_DOWN : TUI_KEY_RIGHT;

    if (event->key == prev)
        return scrollbar_finish(sb, event,
                                scrollbar_user_set(sb, sb->value - 1));

    if (event->key == next)
        return scrollbar_finish(sb, event,
                                scrollbar_user_set(sb, sb->value + 1));

    if (event->key == TUI_KEY_PAGEUP)
        return scrollbar_finish(
            sb, event,
            scrollbar_user_set(sb, sb->value - scrollbar_step(sb)));

    if (event->key == TUI_KEY_PAGEDOWN)
        return scrollbar_finish(
            sb, event,
            scrollbar_user_set(sb, sb->value + scrollbar_step(sb)));

    if (event->key == TUI_KEY_HOME)
        return scrollbar_finish(sb, event,
                                scrollbar_user_set(sb, sb->min));

    if (event->key == TUI_KEY_END)
        return scrollbar_finish(
            sb, event,
            scrollbar_user_set(sb, scrollbar_max_value(sb)));

    return 0;
}

static int scrollbar_event(TuiControl *control, TuiEvent *event)
{
    TuiScrollBar *sb;
    TuiDesktop *desktop;
    int lx;
    int ly;
    int pos;
    int length;
    int thumb;
    int size;
    int changed;

    sb = (TuiScrollBar *)control;

    if (event->type == TUI_EV_KEY)
        return scrollbar_key(sb, event);

    if (event->type != TUI_EV_MOUSE)
        return 0;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return 0;

    tui_control_screen_to_local(control,
                                event->mouse_x,
                                event->mouse_y,
                                &lx, &ly);
    pos = sb->orientation == TUI_HORIZONTAL ? lx : ly;
    length = scrollbar_length(sb);

    if (sb->dragging) {
        if (event->mouse_action == TUI_MOUSE_UP) {
            sb->dragging = 0;

            if (desktop->capture == control)
                tui_desktop_clear_capture(desktop);

            return 1;
        }

        if (event->mouse_action == TUI_MOUSE_MOVE) {
            changed = scrollbar_user_set(
                sb,
                scrollbar_value_from_pos(sb,
                                         pos - 1 - sb->drag_offset));
            return scrollbar_finish(sb, event, changed);
        }

        return 1;
    }

    if (event->mouse_action != TUI_MOUSE_DOWN ||
        !(event->mouse_buttons & TUI_MOUSE_LEFT))
        return 0;

    if (pos < 0 || pos >= length)
        return 0;

    if (pos == 0)
        return scrollbar_finish(sb, event,
                                scrollbar_user_set(sb, sb->value - 1));

    if (pos == length - 1)
        return scrollbar_finish(sb, event,
                                scrollbar_user_set(sb, sb->value + 1));

    thumb = scrollbar_thumb_pos(sb);
    size = scrollbar_thumb_size(sb);

    if (pos - 1 < thumb)
        return scrollbar_finish(
            sb, event,
            scrollbar_user_set(sb, sb->value - scrollbar_step(sb)));

    if (pos - 1 >= thumb + size)
        return scrollbar_finish(
            sb, event,
            scrollbar_user_set(sb, sb->value + scrollbar_step(sb)));

    sb->dragging = 1;
    sb->drag_offset = pos - 1 - thumb;
    tui_desktop_set_capture(desktop, control);

    return 1;
}

void tui_scrollbar_init(TuiScrollBar *scrollbar,
                        int x,
                        int y,
                        int length,
                        int orientation,
                        int command)
{
    int vertical;

    vertical = orientation != TUI_HORIZONTAL;

    tui_control_init(&scrollbar->control,
                     &tui_scrollbar_class,
                     x, y,
                     vertical ? 1 : length,
                     vertical ? length : 1,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    scrollbar->min = 0;
    scrollbar->max = 0;
    scrollbar->value = 0;
    scrollbar->page = 0;
    scrollbar->orientation = vertical ? TUI_VERTICAL : TUI_HORIZONTAL;
    scrollbar->command = command;
    scrollbar->dragging = 0;
    scrollbar->drag_offset = 0;
}

void tui_scrollbar_set_range(TuiScrollBar *scrollbar, int min, int max)
{
    scrollbar->min = min;
    scrollbar->max = max;
    scrollbar_normalize(scrollbar);
}

void tui_scrollbar_set_page(TuiScrollBar *scrollbar, int page)
{
    scrollbar->page = page;
    scrollbar_normalize(scrollbar);
}

void tui_scrollbar_set_value(TuiScrollBar *scrollbar, int value)
{
    scrollbar->value = value;
    scrollbar_normalize(scrollbar);
}

int tui_scrollbar_get_value(const TuiScrollBar *scrollbar)
{
    return scrollbar->value;
}

void tui_scrollbar_set_command(TuiScrollBar *scrollbar, int command)
{
    scrollbar->command = command;
}
