#include "tui_internal.h"

/*
 * ------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------
 */

int tui_strlen(const char *s)
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

int tui_max(int a, int b)
{
    return a > b ? a : b;
}

int tui_min(int a, int b)
{
    return a < b ? a : b;
}


/*
 * ------------------------------------------------------------
 * Drawing primitives
 * ------------------------------------------------------------
 */

/*
 * Converts local coordinates to screen coordinates.
 * Returns 0 if the cell is outside the clipping rectangle.
 */
static int tui_clip_point(const TuiDraw *d, int x, int y,
                          int *sx, int *sy)
{
    *sx = d->ox + x;
    *sy = d->oy + y;

    return !(*sx < d->x1 || *sx >= d->x2 ||
             *sy < d->y1 || *sy >= d->y2);
}

/*
 * Runs of cells (console.h). The MiniCPU backend has its own, which is the reason
 * for having them; any other backend gets these loops.
 */
#ifndef TUI_BACKEND_MMIO
void tui_console_fill(int x, int y, int n, int ch, int attr)
{
    int i;

    for (i = 0; i < n; ++i)
        tui_console_cell(x + i, y, ch, attr);
}

void tui_console_text(int x, int y, const char *text, int n, int attr)
{
    int i;

    for (i = 0; i < n; ++i)
        tui_console_cell(x + i, y, (unsigned char)text[i], attr);
}
#endif

void tui_putc(TuiDraw *d, int x, int y, int ch, int attr)
{
    int sx;
    int sy;

    if (!tui_clip_point(d, x, y, &sx, &sy))
        return;

    tui_console_cell(sx, sy, ch, attr);
}

void tui_draw_cursor(TuiDraw *draw, int x, int y)
{
    int sx;
    int sy;

    if (draw->desktop == 0 ||
        !tui_clip_point(draw, x, y, &sx, &sy))
        return;

    draw->desktop->cursor_x = sx;
    draw->desktop->cursor_y = sy;
    draw->desktop->cursor_visible = 1;
}

void tui_text(TuiDraw *d, int x, int y,
                     const char *s, int attr)
{
    int sx;
    int sy;
    int n;

    if (s == 0)
        return;

    /*
     * Same cells as calling tui_putc for each character, but the row is clipped
     * once and the console gets the whole run. On the MiniCPU every call costs
     * dozens of instructions and a full redraw is mostly text and fills.
     */
    sx = d->ox + x;
    sy = d->oy + y;
    if (sy < d->y1 || sy >= d->y2)
        return;

    /* The characters left of the clip are skipped, not drawn. */
    while (*s != 0 && sx < d->x1) {
        ++sx;
        ++s;
    }

    n = 0;

    while (s[n] != 0 && sx + n < d->x2)
        ++n;

    if (n > 0)
        tui_console_text(sx, sy, s, n, attr);
}

void tui_chars(TuiDraw *d, int x, int y,
               const char *s, int n, int attr)
{
    int sx;
    int sy;
    int from;
    int to;

    sx = d->ox + x;
    sy = d->oy + y;
    if (sy < d->y1 || sy >= d->y2)
        return;

    from = tui_max(sx, d->x1);
    to = tui_min(sx + n, d->x2);

    if (to > from)
        tui_console_text(from, sy, s + (from - sx), to - from, attr);
}

void tui_text_padded(TuiDraw *d, int x, int y, int w,
                     const char *s, int attr)
{
    int sx;
    int sy;
    int start;
    int end;
    int n;

    sx = d->ox + x;
    sy = d->oy + y;
    if (sy < d->y1 || sy >= d->y2)
        return;

    start = tui_max(sx, d->x1);
    end = tui_min(sx + w, d->x2);

    if (start >= end)
        return;

    n = 0;

    if (s != 0) {
        /* The characters left of the clip are skipped, not drawn. */
        for (; sx < start && *s != 0; ++sx)
            ++s;

        while (s[n] != 0 && start + n < end)
            ++n;
    }

    if (n > 0)
        tui_console_text(start, sy, s, n, attr);

    if (start + n < end)
        tui_console_fill(start + n, sy, end - start - n, ' ', attr);
}

void tui_fill(TuiDraw *d, int x, int y,
                     int w, int h, int ch, int attr)
{
    int yy;
    int x1;
    int y1;
    int x2;
    int y2;

    /* Intersect with the clip rectangle once instead of per cell. */
    x1 = tui_max(d->ox + x, d->x1);
    y1 = tui_max(d->oy + y, d->y1);
    x2 = tui_min(d->ox + x + w, d->x2);
    y2 = tui_min(d->oy + y + h, d->y2);

    if (x2 <= x1)
        return;

    for (yy = y1; yy < y2; ++yy)
        tui_console_fill(x1, yy, x2 - x1, ch, attr);
}

void tui_box(TuiDraw *d, int w, int h, int attr)
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

void tui_control_init(TuiControl *control,
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

    control->dock = TUI_DOCK_NONE;

    control->flags = flags;

    control->attr = TUI_ATTR_INHERIT;
}

int tui_control_attr(TuiControl *control)
{
    while (control != 0) {
        if (control->attr != TUI_ATTR_INHERIT)
            return control->attr;

        control = control->parent;
    }

    return TUI_ATTR_WINDOW;
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

    tui_invalidate(child);
}

void tui_bring_to_front(TuiControl *control)
{
    TuiControl *parent;

    parent = control->parent;

    if (parent == 0 || parent->last == control)
        return;

    if (control->prev != 0)
        control->prev->next = control->next;
    else
        parent->first = control->next;

    control->next->prev = control->prev;

    control->prev = parent->last;
    control->next = 0;
    parent->last->next = control;
    parent->last = control;

    tui_invalidate(control);
}

/*
 * Everything that must not outlive a control that leaves the tree: the focus and
 * the mouse capture if they were inside it, and what each class holds on to (see
 * TuiClass.detach). Children first. It runs while the subtree is still attached,
 * because releasing some of it (a combo box's list) needs the desktop.
 */
static void tui_detach_tree(TuiDesktop *desktop, TuiControl *control)
{
    TuiControl *child;
    TuiControl *next;

    for (child = control->first; child != 0; child = next) {
        next = child->next;
        tui_detach_tree(desktop, child);
    }

    if (control->cls != 0 && control->cls->detach != 0)
        control->cls->detach(control);

    if (desktop != 0) {
        if (desktop->focused == control)
            tui_desktop_set_focus(desktop, 0);

        if (desktop->capture == control)
            tui_desktop_clear_capture(desktop);
    }
}

void tui_remove(TuiControl *control)
{
    TuiControl *parent;

    parent = control->parent;

    if (parent == 0)
        return;

    /* While it is still in the tree: that is where its rectangle is. */
    tui_invalidate(control);

    tui_detach_tree(tui_find_desktop(control), control);

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
 * Layout
 * ------------------------------------------------------------
 */

static void tui_layout_children(TuiControl *parent)
{
    TuiControl *child;
    int left;
    int top;
    int right;
    int bottom;

    left = 0;
    top = 0;
    right = parent->width;
    bottom = parent->height;

    if (parent->cls == &tui_window_class) {
        right -= 2;
        bottom -= 2;

        if (right < 0)
            right = 0;

        if (bottom < 0)
            bottom = 0;
    }

    child = parent->first;

    while (child != 0) {

        if ((child->flags & TUI_VISIBLE) == 0) {
            child = child->next;
            continue;
        }

        switch (child->dock) {

            case TUI_DOCK_TOP:
                child->x = left;
                child->y = top;
                child->width = right - left;

                top += child->height;
                break;

            case TUI_DOCK_BOTTOM:
                child->x = left;
                child->y = bottom - child->height;
                child->width = right - left;

                bottom -= child->height;
                break;

            case TUI_DOCK_LEFT:
                child->x = left;
                child->y = top;
                child->height = bottom - top;

                left += child->width;
                break;

            case TUI_DOCK_RIGHT:
                child->x = right - child->width;
                child->y = top;
                child->height = bottom - top;

                right -= child->width;
                break;

            case TUI_DOCK_FILL:
                child->x = left;
                child->y = top;
                child->width = right - left;
                child->height = bottom - top;
                break;

            case TUI_DOCK_NONE:
            default:
                break;
        }

        child = child->next;
    }
}

/*
 * ------------------------------------------------------------
 * Classes
 * ------------------------------------------------------------
 */

static void desktop_draw(TuiControl *control, TuiDraw *draw);
static int desktop_event(TuiControl *control, TuiEvent *event);

static const TuiClass desktop_class = {
    desktop_draw,
    desktop_event,
    0,
    TUI_CLASS_OPAQUE
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
             tui_control_attr(control));
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

    desktop->control.attr = TUI_ATTR_DESKTOP;

    desktop->focused = 0;
    desktop->capture = 0;

    desktop->cursor_visible = 0;
    desktop->cursor_x = 0;
    desktop->cursor_y = 0;

    desktop->dirty_all = 1;
    desktop->dirty_count = 0;
    desktop->frame_count = 0;
    desktop->dirty_serial = 0;
    desktop->commands = 0;
    desktop->command_context = 0;
    desktop->command_status = 0;
    desktop->partial_draw = 0;
    desktop->draw_cover = 0;
    desktop->draw_started = 0;
}

/* The focus moved onto or off 'control': repaint what looks different. */
static void tui_invalidate_focus(TuiControl *control)
{
    if (control->cls != 0 && control->cls->focus_changed != 0)
        control->cls->focus_changed(control);
    else
        tui_invalidate(control);
}

/*
 * A window became active or inactive. Its frame looks different only when it is
 * drawn double while active (see window_draw); for any other window nothing changes.
 */
static void tui_invalidate_active_frame(TuiControl *window)
{
    if (window != 0 &&
        (((TuiWindow *)window)->flags & TUI_WINDOW_ACTIVE_DOUBLE))
        tui_invalidate_frame(window);
}

void tui_desktop_set_focus(TuiDesktop *desktop,
                           TuiControl *control)
{
    TuiControl *old;

    old = desktop->focused;

    if (old == control)
        return;

    desktop->focused = control;

    /* The new focus asks for the cursor again when it is drawn. */
    desktop->cursor_visible = 0;

    if (old != 0)
        tui_invalidate_focus(old);

    if (control != 0)
        tui_invalidate_focus(control);

    if (tui_window_of(old) != tui_window_of(control)) {
        tui_invalidate_active_frame(tui_window_of(old));
        tui_invalidate_active_frame(tui_window_of(control));
    }
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
 * Desktop lookup / focus
 * ------------------------------------------------------------
 */

TuiDesktop *tui_find_desktop(TuiControl *control)
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

int tui_control_has_focus(TuiControl *control)
{
    TuiDesktop *desktop;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return 0;

    return desktop->focused == control;
}


/*
 * ------------------------------------------------------------
 * Drawing tree
 * ------------------------------------------------------------
 */

static void tui_get_child_context(TuiControl *control,
                                  const TuiDraw *draw,
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
    if (control->cls == &tui_window_class) {
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

/*
 * Screen position of the control's top-left corner.
 */
void tui_control_screen_pos(TuiControl *control,
                                   int *sx, int *sy)
{
    TuiDraw d;
    TuiDraw cd;

    if (control->parent == 0) {
        *sx = control->x;
        *sy = control->y;
        return;
    }

    tui_control_screen_pos(control->parent, &d.ox, &d.oy);
    d.x1 = d.ox;
    d.y1 = d.oy;
    d.x2 = d.ox + control->parent->width;
    d.y2 = d.oy + control->parent->height;

    tui_get_child_context(control->parent, &d, &cd);

    *sx = cd.ox + control->x;
    *sy = cd.oy + control->y;
}

void tui_control_screen_to_local(TuiControl *control,
                                 int screen_x, int screen_y,
                                 int *local_x, int *local_y)
{
    int sx;
    int sy;

    tui_control_screen_pos(control, &sx, &sy);

    *local_x = screen_x - sx;
    *local_y = screen_y - sy;
}

/*
 * 'draw' describes the control exactly as in tui_draw_tree().
 */
static TuiControl *tui_hit_control(TuiControl *control,
                                   const TuiDraw *draw,
                                   int x, int y)
{
    TuiControl *child;
    TuiControl *hit;
    TuiDraw children;
    TuiDraw cd;

    if ((control->flags & TUI_VISIBLE) == 0)
        return 0;

    if (x < tui_max(draw->ox, draw->x1) ||
        x >= tui_min(draw->ox + control->width, draw->x2) ||
        y < tui_max(draw->oy, draw->y1) ||
        y >= tui_min(draw->oy + control->height, draw->y2))
        return 0;

    tui_layout_children(control);

    tui_get_child_context(control, draw, &children);

    /* Last child is drawn on top, so it is tested first. */
    child = control->last;

    while (child != 0) {
        cd = children;

        cd.ox += child->x;
        cd.oy += child->y;

        hit = tui_hit_control(child, &cd, x, y);

        if (hit != 0)
            return hit;

        child = child->prev;
    }

    return control;
}

TuiControl *tui_hit_test(TuiControl *root, int x, int y)
{
    TuiDraw draw;

    draw.desktop = 0;

    tui_control_screen_pos(root, &draw.ox, &draw.oy);

    draw.x1 = draw.ox;
    draw.y1 = draw.oy;
    draw.x2 = draw.ox + root->width;
    draw.y2 = draw.oy + root->height;

    return tui_hit_control(root, &draw, x, y);
}

/*
 * The frontmost opaque control that covers the whole region, or 0. 'draw'
 * describes 'control' exactly as in tui_draw_tree(); 'region' only carries the
 * rectangle. A child is looked at only if the area its parent lets it draw in
 * (the inside of a window) holds the region, and if its own rectangle does: then
 * all of the region is painted by it, and by what is drawn after it.
 */
static TuiControl *tui_find_cover(TuiControl *control,
                                  const TuiDraw *draw,
                                  const TuiDraw *region)
{
    TuiControl *child;
    TuiControl *found;
    TuiDraw children;
    TuiDraw cd;

    tui_layout_children(control);
    tui_get_child_context(control, draw, &children);

    if (children.x1 <= region->x1 && children.y1 <= region->y1 &&
        children.x2 >= region->x2 && children.y2 >= region->y2) {

        for (child = control->last; child != 0; child = child->prev) {
            if ((child->flags & TUI_VISIBLE) == 0)
                continue;

            cd = children;
            cd.ox += child->x;
            cd.oy += child->y;

            if (cd.ox > region->x1 || cd.oy > region->y1 ||
                cd.ox + child->width < region->x2 ||
                cd.oy + child->height < region->y2)
                continue;

            found = tui_find_cover(child, &cd, region);

            if (found != 0)
                return found;
        }
    }

    if (control->cls != 0 && (control->cls->flags & TUI_CLASS_OPAQUE))
        return control;

    return 0;
}

static int tui_is_ancestor(TuiControl *ancestor, TuiControl *control)
{
    for (control = control->parent; control != 0; control = control->parent) {
        if (control == ancestor)
            return 1;
    }

    return 0;
}

static void tui_draw_tree(TuiControl *control,
                          TuiDraw *draw)
{
    TuiControl *child;
    TuiDraw children;
    TuiDraw cd;
    int own;

    if ((control->flags & TUI_VISIBLE) == 0)
        return;

    own = 1;

    /*
     * Partial redraw: a control (and so its children, which are clipped to
     * it) that does not touch the region draws nothing, so skip its whole
     * subtree. Costs four comparisons instead of the draw.
     */
    if (draw->desktop != 0 && draw->desktop->partial_draw) {
        if (draw->ox >= draw->x2 ||
            draw->ox + control->width <= draw->x1 ||
            draw->oy >= draw->y2 ||
            draw->oy + control->height <= draw->y1)
            return;

        /*
         * Before the control that covers the region (see tui_draw_region) what
         * is drawn would be covered by it: whole subtrees that come earlier are
         * skipped, and the ancestors only lay out and pass on to their children.
         */
        if (draw->desktop->draw_cover != 0 && !draw->desktop->draw_started) {
            if (control == draw->desktop->draw_cover)
                draw->desktop->draw_started = 1;
            else if (tui_is_ancestor(control, draw->desktop->draw_cover))
                own = 0;
            else
                return;
        }
    }

    /*
     * Calculate child geometry before drawing.
     */
    tui_layout_children(control);

    if (own && control->cls != 0 &&
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

void tui_draw_begin(TuiDesktop *desktop)
{
    desktop->control.width =
        tui_console_width();

    desktop->control.height =
        tui_console_height();

    /*
     * The cursor is not reset here: a partial draw that does not reach the
     * focused control must leave it where it is. A control that has the focus
     * asks for the cursor whenever it is drawn, and a focus change hides it.
     */
}

void tui_draw_region(TuiDesktop *desktop, int x1, int y1, int x2, int y2)
{
    TuiDraw draw;

    draw.desktop = desktop;

    draw.ox = 0;
    draw.oy = 0;

    draw.x1 = tui_max(x1, 0);
    draw.y1 = tui_max(y1, 0);

    draw.x2 = tui_min(x2, desktop->control.width);
    draw.y2 = tui_min(y2, desktop->control.height);

    /* A region covering the whole screen is just the normal full draw. */
    desktop->partial_draw = draw.x1 > 0 || draw.y1 > 0 ||
                            draw.x2 < desktop->control.width ||
                            draw.y2 < desktop->control.height;

    /* Start from the frontmost opaque control that covers it, if there is one. */
    desktop->draw_cover = 0;
    desktop->draw_started = 0;

    if (desktop->partial_draw && draw.x1 < draw.x2 && draw.y1 < draw.y2) {
        desktop->draw_cover = tui_find_cover(&desktop->control, &draw, &draw);

        if (desktop->draw_cover == &desktop->control)
            desktop->draw_cover = 0;
    }

    tui_draw_tree(&desktop->control,
                  &draw);

    desktop->partial_draw = 0;
    desktop->draw_cover = 0;
}

void tui_draw_end(TuiDesktop *desktop)
{
    tui_console_cursor(desktop->cursor_x,
                       desktop->cursor_y,
                       desktop->cursor_visible);

    tui_console_present();
}

void tui_draw(TuiDesktop *desktop)
{
    tui_draw_begin(desktop);

    desktop->cursor_visible = 0;

    tui_draw_region(desktop,
                    0, 0,
                    desktop->control.width,
                    desktop->control.height);

    desktop->dirty_all = 0;
    desktop->dirty_count = 0;
    desktop->frame_count = 0;

    tui_draw_end(desktop);
}

/*
 * ------------------------------------------------------------
 * Commands
 * ------------------------------------------------------------
 */

const TuiCommand *tui_command_find(const TuiCommand *table, int command)
{
    if (table == 0 || command == TUI_CMD_NONE)
        return 0;

    for (; table->command != TUI_CMD_NONE; ++table) {
        if (table->command == command)
            return table;
    }

    return 0;
}

void tui_desktop_set_commands(TuiDesktop *desktop,
                              const TuiCommand *table,
                              void *context,
                              struct TuiStatusBar *status)
{
    desktop->commands = table;
    desktop->command_context = context;
    desktop->command_status = status;
}

int tui_command_run(TuiDesktop *desktop, int command)
{
    const TuiCommand *entry;

    entry = tui_command_find(desktop->commands, command);

    if (entry == 0)
        return 0;

    if (entry->text != 0 && desktop->command_status != 0)
        tui_statusbar_set_text(desktop->command_status, entry->text);

    if (entry->run != 0)
        entry->run(desktop->command_context, command);

    return 1;
}

/*
 * ------------------------------------------------------------
 * Invalidation
 * ------------------------------------------------------------
 */

static void tui_dirty_add(TuiDesktop *desktop,
                          int x1, int y1, int x2, int y2)
{
    int i;
    int j;
    int kept;
    int best;
    int best_area;
    int area;
    int ux1;
    int uy1;
    int ux2;
    int uy2;

    ++desktop->dirty_serial;

    if (desktop->dirty_all)
        return;

    x1 = tui_max(x1, 0);
    y1 = tui_max(y1, 0);
    x2 = tui_min(x2, desktop->control.width);
    y2 = tui_min(y2, desktop->control.height);

    if (x1 >= x2 || y1 >= y2)
        return;

    /* Already covered by something we are going to draw. */
    for (i = 0; i < desktop->dirty_count; ++i) {
        if (desktop->dirty[i][0] <= x1 && desktop->dirty[i][1] <= y1 &&
            desktop->dirty[i][2] >= x2 && desktop->dirty[i][3] >= y2)
            return;
    }

    /*
     * Rectangles that overlap would paint the overlap twice, and painting a
     * region is what costs. Take their union when it is no bigger than the two
     * together: a window dragged a few cells leaves its old and its new place
     * almost on top of each other, and that is one rectangle, not two. The union
     * can now reach others, so start over after each merge; every merge frees a
     * slot, so this ends. The order of the list does not matter.
     */
    i = 0;
    while (i < desktop->dirty_count) {
        ux1 = tui_min(x1, desktop->dirty[i][0]);
        uy1 = tui_min(y1, desktop->dirty[i][1]);
        ux2 = tui_max(x2, desktop->dirty[i][2]);
        uy2 = tui_max(y2, desktop->dirty[i][3]);

        if (x1 < desktop->dirty[i][2] && desktop->dirty[i][0] < x2 &&
            y1 < desktop->dirty[i][3] && desktop->dirty[i][1] < y2 &&
            (ux2 - ux1) * (uy2 - uy1) <=
                (x2 - x1) * (y2 - y1) +
                (desktop->dirty[i][2] - desktop->dirty[i][0]) *
                (desktop->dirty[i][3] - desktop->dirty[i][1])) {
            x1 = ux1;
            y1 = uy1;
            x2 = ux2;
            y2 = uy2;

            --desktop->dirty_count;
            for (j = 0; j < 4; ++j)
                desktop->dirty[i][j] = desktop->dirty[desktop->dirty_count][j];

            i = 0;
        } else {
            ++i;
        }
    }

    /* The new one swallows the ones it covers, which frees their slots. */
    kept = 0;
    for (i = 0; i < desktop->dirty_count; ++i) {
        if (x1 <= desktop->dirty[i][0] && y1 <= desktop->dirty[i][1] &&
            x2 >= desktop->dirty[i][2] && y2 >= desktop->dirty[i][3])
            continue;

        if (kept != i) {
            for (j = 0; j < 4; ++j)
                desktop->dirty[kept][j] = desktop->dirty[i][j];
        }
        ++kept;
    }
    desktop->dirty_count = kept;

    /* A frame inside the new rectangle is repainted with it anyway. */
    kept = 0;
    for (i = 0; i < desktop->frame_count; ++i) {
        if (x1 <= desktop->frames[i][0] && y1 <= desktop->frames[i][1] &&
            x2 >= desktop->frames[i][2] && y2 >= desktop->frames[i][3])
            continue;

        if (kept != i) {
            for (j = 0; j < 4; ++j)
                desktop->frames[kept][j] = desktop->frames[i][j];
        }
        ++kept;
    }
    desktop->frame_count = kept;

    /*
     * No room: grow the rectangle that would grow the least to take this one.
     * Repainting a little more is cheap; giving up and repainting the whole
     * screen is not (coming from a control in another window, a click can mark
     * two frames, raise nested windows and open a list in one go).
     */
    if (desktop->dirty_count >= TUI_DIRTY_MAX) {
        best = 0;
        best_area = -1;

        for (i = 0; i < desktop->dirty_count; ++i) {
            area = (tui_max(x2, desktop->dirty[i][2]) -
                    tui_min(x1, desktop->dirty[i][0])) *
                   (tui_max(y2, desktop->dirty[i][3]) -
                    tui_min(y1, desktop->dirty[i][1]));

            if (best_area < 0 || area < best_area) {
                best_area = area;
                best = i;
            }
        }

        desktop->dirty[best][0] = tui_min(x1, desktop->dirty[best][0]);
        desktop->dirty[best][1] = tui_min(y1, desktop->dirty[best][1]);
        desktop->dirty[best][2] = tui_max(x2, desktop->dirty[best][2]);
        desktop->dirty[best][3] = tui_max(y2, desktop->dirty[best][3]);
        return;
    }

    desktop->dirty[desktop->dirty_count][0] = x1;
    desktop->dirty[desktop->dirty_count][1] = y1;
    desktop->dirty[desktop->dirty_count][2] = x2;
    desktop->dirty[desktop->dirty_count][3] = y2;
    ++desktop->dirty_count;
}

void tui_invalidate_rect(TuiControl *control,
                         int x, int y, int width, int height)
{
    TuiDesktop *desktop;
    TuiControl *ancestor;
    int sx;
    int sy;
    int ax;
    int ay;
    int x1;
    int y1;
    int x2;
    int y2;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return;

    tui_control_screen_pos(control, &sx, &sy);

    x1 = sx + x;
    y1 = sy + y;
    x2 = x1 + width;
    y2 = y1 + height;

    /*
     * A window only lets its children show inside its border (see
     * tui_get_child_context), so what lies outside it changes nothing on screen.
     * Leaving it in would repaint, layer by layer from the bottom up, whatever is
     * there: a window dragged past the edge of its parent would flash the windows
     * beneath it outside the parent until the ones on top were drawn again. The
     * rectangle may end up empty, which is still an invalidation (the event was
     * handled and reported what it changed, namely nothing visible).
     */
    for (ancestor = control->parent; ancestor != 0; ancestor = ancestor->parent) {
        if (ancestor->cls != &tui_window_class)
            continue;

        tui_control_screen_pos(ancestor, &ax, &ay);

        x1 = tui_max(x1, ax + 1);
        y1 = tui_max(y1, ay + 1);
        x2 = tui_min(x2, ax + ancestor->width - 1);
        y2 = tui_min(y2, ay + ancestor->height - 1);
    }

    tui_dirty_add(desktop, x1, y1, x2, y2);
}

void tui_invalidate(TuiControl *control)
{
    tui_invalidate_rect(control, 0, 0, control->width, control->height);
}

/*
 * Only the outer ring of cells of a control. The children of a window live inside
 * the border, so repainting the ring does not touch them: far cheaper than the
 * whole control when just the frame changed (the active window, a title). It is
 * one entry of its own list and not four rectangles; tui_draw_pending() expands
 * it into the four strips.
 */
void tui_invalidate_frame(TuiControl *control)
{
    TuiDesktop *desktop;
    int x1;
    int y1;
    int x2;
    int y2;
    int i;

    if (control == 0)
        return;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return;

    tui_control_screen_pos(control, &x1, &y1);
    x2 = x1 + control->width;
    y2 = y1 + control->height;

    /* No ring to speak of, or it is cut by the screen: take the whole thing. */
    if (control->width < 3 || control->height < 3 ||
        x1 < 0 || y1 < 0 ||
        x2 > desktop->control.width || y2 > desktop->control.height) {
        tui_invalidate(control);
        return;
    }

    ++desktop->dirty_serial;

    if (desktop->dirty_all)
        return;

    /* Inside something already pending, or already listed. */
    for (i = 0; i < desktop->dirty_count; ++i) {
        if (desktop->dirty[i][0] <= x1 && desktop->dirty[i][1] <= y1 &&
            desktop->dirty[i][2] >= x2 && desktop->dirty[i][3] >= y2)
            return;
    }

    for (i = 0; i < desktop->frame_count; ++i) {
        if (desktop->frames[i][0] == x1 && desktop->frames[i][1] == y1 &&
            desktop->frames[i][2] == x2 && desktop->frames[i][3] == y2)
            return;
    }

    /* No room for another frame: it becomes a rectangle, which can merge. */
    if (desktop->frame_count >= TUI_FRAME_MAX) {
        tui_dirty_add(desktop, x1, y1, x2, y2);
        return;
    }

    desktop->frames[desktop->frame_count][0] = x1;
    desktop->frames[desktop->frame_count][1] = y1;
    desktop->frames[desktop->frame_count][2] = x2;
    desktop->frames[desktop->frame_count][3] = y2;
    ++desktop->frame_count;
}

void tui_invalidate_all(TuiDesktop *desktop)
{
    ++desktop->dirty_serial;
    desktop->dirty_all = 1;
}

void tui_event_done(TuiControl *control)
{
    TuiDesktop *desktop;

    desktop = tui_find_desktop(control);

    if (desktop != 0)
        ++desktop->dirty_serial;
}

void tui_draw_pending(TuiDesktop *desktop)
{
    int i;

    if (desktop->dirty_all) {
        tui_draw(desktop);
        return;
    }

    if (desktop->dirty_count == 0 && desktop->frame_count == 0)
        return;

    tui_draw_begin(desktop);

    for (i = 0; i < desktop->dirty_count; ++i)
        tui_draw_region(desktop,
                        desktop->dirty[i][0], desktop->dirty[i][1],
                        desktop->dirty[i][2], desktop->dirty[i][3]);

    /* A frame is its four strips: top, bottom, and the sides between them. */
    for (i = 0; i < desktop->frame_count; ++i) {
        tui_draw_region(desktop,
                        desktop->frames[i][0], desktop->frames[i][1],
                        desktop->frames[i][2], desktop->frames[i][1] + 1);
        tui_draw_region(desktop,
                        desktop->frames[i][0], desktop->frames[i][3] - 1,
                        desktop->frames[i][2], desktop->frames[i][3]);
        tui_draw_region(desktop,
                        desktop->frames[i][0], desktop->frames[i][1] + 1,
                        desktop->frames[i][0] + 1, desktop->frames[i][3] - 1);
        tui_draw_region(desktop,
                        desktop->frames[i][2] - 1, desktop->frames[i][1] + 1,
                        desktop->frames[i][2], desktop->frames[i][3] - 1);
    }

    desktop->dirty_count = 0;
    desktop->frame_count = 0;

    tui_draw_end(desktop);
}

TuiControl *tui_window_of(TuiControl *control)
{
    for (; control != 0; control = control->parent) {
        if (control->cls == &tui_window_class)
            return control;
    }

    return 0;
}

void tui_control_rect(TuiControl *control,
                      int *x1, int *y1, int *x2, int *y2)
{
    tui_control_screen_pos(control, x1, y1);
    *x2 = *x1 + control->width;
    *y2 = *y1 + control->height;
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

/*
 * Mouse DOWN/DOUBLE side effects: raise the floating window under
 * the mouse and focus the nearest focusable control.
 */
/* True when 'control' is 'ancestor' or one of its descendants. */
static int tui_is_within(TuiControl *control, TuiControl *ancestor)
{
    for (; control != 0; control = control->parent) {
        if (control == ancestor)
            return 1;
    }

    return 0;
}

/*
 * A click on a window surface with no focusable control under it
 * activates that window: focus goes to its first focusable
 * descendant, or to the window itself when it has none. A window
 * that already holds the focus is left alone.
 */
static void tui_focus_window(TuiDesktop *desktop, TuiControl *window)
{
    TuiControl *c;

    if (tui_is_within(desktop->focused, window))
        return;

    for (c = window->first; c != 0; c = tui_tree_next(window, c)) {
        if (tui_is_focusable(c)) {
            tui_desktop_set_focus(desktop, c);
            return;
        }
    }

    tui_desktop_set_focus(desktop, window);
}

static void tui_mouse_down(TuiDesktop *desktop, TuiControl *target)
{
    TuiControl *c;
    int focused;

    focused = 0;

    for (c = target; c != 0; c = c->parent) {
        if (!focused && tui_is_focusable(c)) {
            tui_desktop_set_focus(desktop, c);
            focused = 1;
        }

        if (!focused && c->cls == &tui_window_class) {
            tui_focus_window(desktop, c);
            focused = 1;
        }

        if (c->cls == &tui_window_class &&
            c->dock == TUI_DOCK_NONE)
            tui_bring_to_front(c);
    }
}

/* Offers a mouse event to 'target' and then to each of its parents. */
static int tui_bubble_mouse(TuiControl *target, TuiEvent *event)
{
    while (target != 0) {
        if (target->cls != 0 &&
            target->cls->event != 0 &&
            target->cls->event(target, event))
            return 1;

        target = target->parent;
    }

    return 0;
}

static int tui_dispatch_mouse(TuiDesktop *desktop,
                              TuiEvent *event)
{
    TuiControl *target;

    if (desktop->capture != 0) {
        target = desktop->capture;
    } else {
        target = tui_hit_test(&desktop->control,
                              event->mouse_x,
                              event->mouse_y);

        if (target != 0 &&
            (event->mouse_action == TUI_MOUSE_DOWN ||
             event->mouse_action == TUI_MOUSE_DOUBLE))
            tui_mouse_down(desktop, target);
    }

    /* Bubble up until a control handles it. */
    if (tui_bubble_mouse(target, event))
        return 1;

    /*
     * The console turns a second quick click on the same cell into DOUBLE. Text
     * controls and lists use that, but a button, a check box or a menu title only
     * know about a press, and would lose the click. A double click nobody wanted
     * is a press, offered again.
     */
    if (event->mouse_action == TUI_MOUSE_DOUBLE) {
        event->mouse_action = TUI_MOUSE_DOWN;

        if (tui_bubble_mouse(target, event))
            return 1;

        event->mouse_action = TUI_MOUSE_DOUBLE;
    }

    return 0;
}

static int tui_dispatch_event(TuiDesktop *desktop,
                              TuiEvent *event)
{
    TuiControl *target;

    if (event->type == TUI_EV_MOUSE)
        return tui_dispatch_mouse(desktop, event);

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

        tui_desktop_set_focus(desktop,
                              tui_next_focusable(desktop,
                                                 desktop->focused));
        tui_event_done(&desktop->control);

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

int tui_dispatch(TuiDesktop *desktop,
                 TuiEvent *event)
{
    int serial;
    int handled;

    serial = desktop->dirty_serial;

    handled = tui_dispatch_event(desktop, event);

    /* The event turned into a command: the registered table says what it means. */
    if (handled && event->type == TUI_EV_COMMAND)
        tui_command_run(desktop, event->command);

    /*
     * Handled, yet nothing was invalidated: the control does not report what
     * it changes, so assume anything. Safe, just not fast.
     */
    if (handled && desktop->dirty_serial == serial)
        tui_invalidate_all(desktop);

    return handled;
}

/*
 * ------------------------------------------------------------
 * Input
 * ------------------------------------------------------------
 */

/*
 * The console keeps the details of a mouse event only until the next one, so an
 * event is built the moment its key code comes back.
 */
static void tui_make_event(TuiEvent *event, int key)
{
    event->type = TUI_EV_KEY;
    event->key = key;

    event->command = TUI_CMD_NONE;
    event->source = 0;

    event->mouse_x = 0;
    event->mouse_y = 0;
    event->mouse_action = 0;
    event->mouse_buttons = 0;

    if (event->key == TUI_KEY_MOUSE) {
        event->type = TUI_EV_MOUSE;
        event->key = TUI_KEY_NONE;

        tui_console_mouse(&event->mouse_x,
                          &event->mouse_y,
                          &event->mouse_action,
                          &event->mouse_buttons);
    }
}

/* An event read ahead that was not a mouse move, kept for the next call. */
static TuiEvent tui_lookahead;
static int tui_lookahead_valid;

/*
 * Mouse moves that arrive back to back are one: only where the pointer ends up
 * matters, and every one that is dispatched costs a repaint, which takes longer
 * than the moves take to arrive (a window being dragged would trail behind the
 * mouse). Nothing else is merged: keys, presses and releases all get through, in
 * order. Whatever follows the last move is kept for the next call.
 */
int tui_read_event(TuiEvent *event)
{
    int key;

    if (tui_lookahead_valid) {
        *event = tui_lookahead;
        tui_lookahead_valid = 0;
    } else {
        tui_make_event(event, tui_console_key());
    }

    while (event->type == TUI_EV_MOUSE &&
           event->mouse_action == TUI_MOUSE_MOVE) {
        key = tui_console_poll();

        if (key == TUI_KEY_NONE)
            break;

        tui_make_event(&tui_lookahead, key);

        if (tui_lookahead.type == TUI_EV_MOUSE &&
            tui_lookahead.mouse_action == TUI_MOUSE_MOVE) {
            *event = tui_lookahead;
        } else {
            tui_lookahead_valid = 1;
            break;
        }
    }

    return 1;
}


/*
 * ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------
 */

int tui_init(void)
{
    tui_lookahead_valid = 0;

    return tui_console_init();
}

void tui_shutdown(void)
{
    tui_console_shutdown();
}
