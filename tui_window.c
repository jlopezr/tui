#include "tui_internal.h"

static void window_draw(TuiControl *control, TuiDraw *draw);
static int window_event(TuiControl *control, TuiEvent *event);

const TuiClass tui_window_class = {
    window_draw,
    window_event
};

/*
 * ------------------------------------------------------------
 * Window
 * ------------------------------------------------------------
 */

static void window_draw(TuiControl *control, TuiDraw *draw)
{
    TuiWindow *window;
    int attr;
    int title_len;
    int title_x;

    window = (TuiWindow *)control;

    attr = tui_control_attr(control);

    tui_fill(draw,
             0, 0,
             control->width,
             control->height,
             ' ',
             attr);

    tui_box(draw,
            control->width,
            control->height,
            attr);

    if (window->title != 0) {
        if (window->title_attr != TUI_ATTR_INHERIT)
            attr = window->title_attr;
        title_len = tui_strlen(window->title);
        title_x = (control->width - title_len - 2) / 2;

        if (title_x < 1)
            title_x = 1;

        tui_putc(draw, title_x, 0, ' ', attr);

        tui_text(draw,
                 title_x + 1,
                 0,
                 window->title,
                 attr);

        tui_putc(draw,
                 title_x + title_len + 1,
                 0,
                 ' ',
                 attr);
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

    window = (TuiWindow *)control;

    sx = event->mouse_x - window->drag_dx;
    sy = event->mouse_y - window->drag_dy;

    /* Keep a useful part of the title bar on the desktop. */
    sx = tui_max(sx, 4 - control->width);
    sx = tui_min(sx, desktop->control.width - 4);
    sy = tui_max(sy, 0);
    sy = tui_min(sy, desktop->control.height - 1);

    /* Parent's client origin, in screen coordinates. */
    px = 0;
    py = 0;

    if (control->parent != 0) {
        tui_control_screen_pos(control->parent, &px, &py);

        if (control->parent->cls == &tui_window_class) {
            px += 1;
            py += 1;
        }
    }

    control->x = sx - px;
    control->y = sy - py;
}

static int window_event(TuiControl *control, TuiEvent *event)
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
        control->dock == TUI_DOCK_NONE) {

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
}


