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
    if (s == 0)
        return;

    while (*s != 0) {
        tui_putc(d, x, y, (unsigned char)*s, attr);
        ++x;
        ++s;
    }
}

void tui_fill(TuiDraw *d, int x, int y,
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
    desktop_event
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

static void tui_draw_tree(TuiControl *control,
                          TuiDraw *draw)
{
    TuiControl *child;
    TuiDraw children;
    TuiDraw cd;

    if ((control->flags & TUI_VISIBLE) == 0)
        return;
    
    /*
     * Calculate child geometry before drawing.
     */
    tui_layout_children(control);

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

    desktop->cursor_visible = 0;

    draw.desktop = desktop;

    draw.ox = 0;
    draw.oy = 0;

    draw.x1 = 0;
    draw.y1 = 0;

    draw.x2 = desktop->control.width;
    draw.y2 = desktop->control.height;

    tui_draw_tree(&desktop->control,
                  &draw);

    tui_console_cursor(desktop->cursor_x,
                       desktop->cursor_y,
                       desktop->cursor_visible);

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

/*
 * Mouse DOWN/DOUBLE side effects: raise the floating window under
 * the mouse and focus the nearest focusable control.
 */
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

        if (c->cls == &tui_window_class &&
            c->dock == TUI_DOCK_NONE)
            tui_bring_to_front(c);
    }
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
    while (target != 0) {
        if (target->cls != 0 &&
            target->cls->event != 0 &&
            target->cls->event(target, event))
            return 1;

        target = target->parent;
    }

    return 0;
}

int tui_dispatch(TuiDesktop *desktop,
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
