#include "tui_internal.h"

static void window_draw(TuiControl *control, TuiDraw *draw);
static int window_event(TuiControl *control, TuiEvent *event);
static void window_detach(TuiControl *control);

const TuiClass tui_window_class = {
    window_draw,
    window_event,
    window_detach,
    TUI_CLASS_OPAQUE
};

/* A window that leaves the tree is not being dragged any more. */
static void window_detach(TuiControl *control)
{
    ((TuiWindow *)control)->dragging = 0;
}

/*
 * ------------------------------------------------------------
 * Window
 * ------------------------------------------------------------
 */

/* True when the focused control is the window or one of its descendants. */
static int window_is_active(TuiControl *control)
{
    TuiDesktop *desktop;
    TuiControl *c;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return 0;

    for (c = desktop->focused; c != 0; c = c->parent) {
        if (c == control)
            return 1;
    }

    return 0;
}

static void window_frame(TuiDraw *draw, int w, int h, int attr,
                         int double_line)
{
    int hl;
    int vl;
    int tl;
    int tr;
    int bl;
    int br;
    int x;
    int y;

    if (w < 2 || h < 2)
        return;

    hl = double_line ? TUI_CH_DHLINE : TUI_CH_HLINE;
    vl = double_line ? TUI_CH_DVLINE : TUI_CH_VLINE;
    tl = double_line ? TUI_CH_DTL : TUI_CH_TL;
    tr = double_line ? TUI_CH_DTR : TUI_CH_TR;
    bl = double_line ? TUI_CH_DBL : TUI_CH_BL;
    br = double_line ? TUI_CH_DBR : TUI_CH_BR;

    tui_putc(draw, 0,     0,     tl, attr);
    tui_putc(draw, w - 1, 0,     tr, attr);
    tui_putc(draw, 0,     h - 1, bl, attr);
    tui_putc(draw, w - 1, h - 1, br, attr);

    for (x = 1; x < w - 1; ++x) {
        tui_putc(draw, x, 0,     hl, attr);
        tui_putc(draw, x, h - 1, hl, attr);
    }

    for (y = 1; y < h - 1; ++y) {
        tui_putc(draw, 0,     y, vl, attr);
        tui_putc(draw, w - 1, y, vl, attr);
    }
}

/*
 * X of the space that precedes the title, or -1 when it does not
 * fit. len is the visible title length, already truncated.
 */
static int window_title_x(const TuiWindow *window, int width, int len)
{
    int block;
    int x;

    block = len + 2;

    switch (window->flags & TUI_WINDOW_TITLE_MASK) {
    case TUI_WINDOW_TITLE_LEFT:
        x = 2;
        break;

    case TUI_WINDOW_TITLE_RIGHT:
        x = width - 2 - block;
        break;

    default:
        x = (width - block) / 2;
        break;
    }

    if (x > width - 1 - block)
        x = width - 1 - block;

    if (x < 1)
        x = 1;

    return x;
}

static void window_draw(TuiControl *control, TuiDraw *draw)
{
    TuiWindow *window;
    int attr;
    int title_len;
    int title_x;
    int i;

    window = (TuiWindow *)control;

    attr = tui_control_attr(control);

    /*
     * The frame writes every cell of the border, so only the inside is filled: a
     * repaint of just the ring (the window became active or inactive) would
     * otherwise write each of its cells twice.
     */
    if (control->width >= 2 && control->height >= 2)
        tui_fill(draw,
                 1, 1,
                 control->width - 2,
                 control->height - 2,
                 ' ',
                 attr);
    else
        tui_fill(draw,
                 0, 0,
                 control->width,
                 control->height,
                 ' ',
                 attr);

    window_frame(draw,
                 control->width,
                 control->height,
                 attr,
                 (window->flags & TUI_WINDOW_ACTIVE_DOUBLE) &&
                 window_is_active(control));

    if (window->title != 0 && window->title[0] != '\0') {
        if (window->title_attr != TUI_ATTR_INHERIT)
            attr = window->title_attr;

        /* Keep the corners free: " title " must fit in width - 2. */
        title_len = tui_min(tui_strlen(window->title),
                            control->width - 4);

        if (title_len < 1)
            return;

        title_x = window_title_x(window, control->width, title_len);

        tui_putc(draw, title_x, 0, ' ', attr);

        for (i = 0; i < title_len; ++i)
            tui_putc(draw, title_x + 1 + i, 0,
                     (unsigned char)window->title[i], attr);

        tui_putc(draw, title_x + title_len + 1, 0, ' ', attr);
    }
}

static void window_drag_to(TuiControl *control,
                          TuiDesktop *desktop,
                          TuiEvent *event)
{
    TuiWindow *window;
    int sx;
    int sy;
    int px;
    int py;
    int min_x;
    int max_x;
    int min_y;
    int max_y;

    window = (TuiWindow *)control;

    sx = event->mouse_x - window->drag_dx;
    sy = event->mouse_y - window->drag_dy;

    /* Parent's client origin, in screen coordinates. */
    px = 0;
    py = 0;

    /* Keep a useful part of the title bar on the desktop. */
    min_x = 4 - control->width;
    max_x = desktop->control.width - 4;
    min_y = 0;
    max_y = desktop->control.height - 1;

    if (control->parent != 0) {
        tui_control_screen_pos(control->parent, &px, &py);

        /*
         * And inside the parent when it is a window: it only shows its children
         * within its border, so a window dragged out of it entirely would vanish
         * and could not be grabbed again. The same four cells of title stay in
         * view, on a row of its client area.
         */
        if (control->parent->cls == &tui_window_class) {
            px += 1;
            py += 1;

            min_x = tui_max(min_x, px + 4 - control->width);
            max_x = tui_min(max_x, px + control->parent->width - 2 - 4);
            min_y = tui_max(min_y, py);
            max_y = tui_min(max_y, py + control->parent->height - 2 - 1);
        }
    }

    sx = tui_max(sx, min_x);
    sx = tui_min(sx, max_x);
    sy = tui_max(sy, min_y);
    sy = tui_min(sy, max_y);

    /* Old place and new place. */
    tui_invalidate(control);

    control->x = sx - px;
    control->y = sy - py;

    tui_invalidate(control);
}

static void window_cancel_drag(TuiWindow *window, TuiDesktop *desktop)
{
    window->dragging = 0;

    if (desktop->capture == &window->control)
        tui_desktop_clear_capture(desktop);
}

static int window_handle(TuiControl *control, TuiEvent *event);

/* Starting or ending a drag changes nothing on screen; moving does (above). */
static int window_event(TuiControl *control, TuiEvent *event)
{
    int handled;

    handled = window_handle(control, event);

    if (handled)
        tui_event_done(control);

    return handled;
}

static int window_handle(TuiControl *control, TuiEvent *event)
{
    TuiWindow *window;
    TuiDesktop *desktop;
    int lx;
    int ly;

    if (event->type != TUI_EV_MOUSE)
        return 0;

    window = (TuiWindow *)control;
    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return 0;

    if (window->dragging && (window->flags & TUI_WINDOW_FIXED)) {
        window_cancel_drag(window, desktop);
        return 1;
    }

    if (window->dragging) {
        if (event->mouse_action == TUI_MOUSE_MOVE) {
            window_drag_to(control, desktop, event);
        } else if (event->mouse_action == TUI_MOUSE_UP &&
                   (event->mouse_buttons & TUI_MOUSE_LEFT)) {
            window->dragging = 0;

            if (desktop->capture == control)
                tui_desktop_clear_capture(desktop);
        }

        return 1;
    }

    if (event->mouse_action == TUI_MOUSE_DOWN &&
        (event->mouse_buttons & TUI_MOUSE_LEFT) &&
        control->dock == TUI_DOCK_NONE &&
        !(window->flags & TUI_WINDOW_FIXED)) {

        tui_control_screen_to_local(control,
                                    event->mouse_x,
                                    event->mouse_y,
                                    &lx, &ly);

        if (ly == 0 && lx >= 0 && lx < control->width) {
            window->dragging = 1;
            window->drag_dx = lx;
            window->drag_dy = ly;
            tui_desktop_set_capture(desktop, control);
            return 1;
        }
    }

    return 0;
}

void tui_window_init(TuiWindow *window,
                     int x, int y,
                     int width, int height,
                     const char *title)
{
    tui_control_init(&window->control,
                     &tui_window_class,
                     x, y,
                     width, height,
                     TUI_VISIBLE | TUI_ENABLED);

    window->title = title;

    window->dragging = 0;
    window->drag_dx = 0;
    window->drag_dy = 0;

    window->control.attr = TUI_ATTR_WINDOW;
    window->title_attr = TUI_ATTR_INHERIT;
    window->flags = 0;
}

void tui_window_set_flags(TuiWindow *window, unsigned flags)
{
    TuiDesktop *desktop;

    window->flags = flags;
    tui_invalidate_frame(&window->control);     /* the flags only change the frame */

    if (window->dragging && (flags & TUI_WINDOW_FIXED)) {
        desktop = tui_find_desktop(&window->control);

        if (desktop != 0)
            window_cancel_drag(window, desktop);
        else
            window->dragging = 0;
    }
}

unsigned tui_window_get_flags(const TuiWindow *window)
{
    return window->flags;
}

void tui_window_add_flags(TuiWindow *window, unsigned flags)
{
    tui_window_set_flags(window, window->flags | flags);
}

void tui_window_remove_flags(TuiWindow *window, unsigned flags)
{
    tui_window_set_flags(window, window->flags & ~flags);
}


