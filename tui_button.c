#include "tui_internal.h"

static void button_draw(TuiControl *control, TuiDraw *draw);
static int button_event(TuiControl *control, TuiEvent *event);
static void button_detach(TuiControl *control);

static const TuiClass button_class = {
    button_draw,
    button_event,
    button_detach,
    TUI_CLASS_OPAQUE
};

/* A button that leaves the tree is not held down any more. */
static void button_detach(TuiControl *control)
{
    ((TuiButton *)control)->pressed = 0;
}

/*
 * ------------------------------------------------------------
 * Button
 * ------------------------------------------------------------
 */

static void button_draw(TuiControl *control, TuiDraw *draw)
{
    TuiButton *button;
    int attr;
    int len;
    int x;

    button = (TuiButton *)control;

    if (tui_control_has_focus(control))
        attr = TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);
    else
        attr = TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);

    tui_fill(draw,
             0, 0,
             control->width, 1,
             ' ',
             attr);

    if (control->width <= 0)
        return;

    len = tui_strlen(button->text);

    x = (control->width - len - 2) / 2;

    if (x < 0)
        x = 0;

    tui_putc(draw, x, 0, '[', attr);

    tui_text(draw,
             x + 1, 0,
             button->text,
             attr);

    tui_putc(draw,
             x + len + 1,
             0,
             ']',
             attr);
}

/* Shared by keyboard and mouse activation. */
static int tui_button_command(TuiButton *button,
                              TuiControl *control,
                              TuiEvent *event)
{
    event->type = TUI_EV_COMMAND;
    event->command = button->command;
    event->source = control;

    return 1;
}

static int button_handle(TuiControl *control, TuiEvent *event);

/*
 * Handled events count as reported even when nothing here changed (a press
 * on a button, a mouse move over an open list): the controls that did change
 * something have invalidated it themselves.
 */
static int button_event(TuiControl *control, TuiEvent *event)
{
    int handled;

    handled = button_handle(control, event);

    if (handled)
        tui_event_done(control);

    return handled;
}

static int button_handle(TuiControl *control, TuiEvent *event)
{
    TuiButton *button;
    TuiDesktop *desktop;
    int lx;
    int ly;
    int inside;

    button = (TuiButton *)control;

    if (event->type == TUI_EV_KEY &&
        (event->key == TUI_KEY_ENTER || event->key == ' '))
        return tui_button_command(button, control, event);

    if (event->type != TUI_EV_MOUSE ||
        !(event->mouse_buttons & TUI_MOUSE_LEFT))
        return 0;

    desktop = tui_find_desktop(control);

    if (desktop == 0)
        return 0;

    /* The button keeps capture between DOWN and UP. */
    if (event->mouse_action == TUI_MOUSE_DOWN) {
        button->pressed = 1;
        tui_desktop_set_capture(desktop, control);
        return 1;
    }

    if (event->mouse_action == TUI_MOUSE_UP && button->pressed) {
        button->pressed = 0;

        if (desktop->capture == control)
            tui_desktop_clear_capture(desktop);

        tui_control_screen_to_local(control,
                                    event->mouse_x,
                                    event->mouse_y,
                                    &lx, &ly);

        inside = lx >= 0 && lx < control->width &&
                 ly >= 0 && ly < control->height;

        if (inside)
            return tui_button_command(button, control, event);

        return 1;
    }

    return 0;
}

void tui_button_init(TuiButton *button,
                     int x, int y,
                     int width,
                     const char *text,
                     int command)
{
    tui_control_init(&button->control,
                     &button_class,
                     x, y,
                     width, 1,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    button->text = text;
    button->command = command;
    button->pressed = 0;
}

