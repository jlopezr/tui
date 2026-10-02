#include "tui_internal.h"

static void checkbox_draw(TuiControl *control, TuiDraw *draw);
static int checkbox_event(TuiControl *control, TuiEvent *event);

static const TuiClass checkbox_class = {
    checkbox_draw,
    checkbox_event
};

static int checkbox_attr(TuiControl *control)
{
    if (tui_control_has_focus(control))
        return TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);

    return TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);
}

static void checkbox_draw(TuiControl *control, TuiDraw *draw)
{
    TuiCheckBox *checkbox;
    int attr;
    int x;
    const char *text;

    checkbox = (TuiCheckBox *)control;
    attr = checkbox_attr(control);

    tui_fill(draw, 0, 0, control->width, 1, ' ', attr);

    if (control->width <= 0)
        return;

    tui_putc(draw, 0, 0, '[', attr);

    if (control->width > 1)
        tui_putc(draw, 1, 0, checkbox->checked ? 'x' : ' ', attr);

    if (control->width > 2)
        tui_putc(draw, 2, 0, ']', attr);

    text = checkbox->text;
    x = 4;

    while (text != 0 &&
           *text != '\0' &&
           x < control->width) {
        tui_putc(draw, x, 0, (unsigned char)*text, attr);
        ++text;
        ++x;
    }
}

static int checkbox_toggle(TuiCheckBox *checkbox,
                           TuiControl *control,
                           TuiEvent *event)
{
    checkbox->checked = !checkbox->checked;

    if (checkbox->command == TUI_CMD_NONE)
        return 1;

    event->type = TUI_EV_COMMAND;
    event->command = checkbox->command;
    event->source = control;

    return 1;
}

static int checkbox_event(TuiControl *control, TuiEvent *event)
{
    TuiCheckBox *checkbox;

    checkbox = (TuiCheckBox *)control;

    if (event->type == TUI_EV_MOUSE &&
        event->mouse_action == TUI_MOUSE_DOWN &&
        (event->mouse_buttons & TUI_MOUSE_LEFT))
        return checkbox_toggle(checkbox, control, event);

    if (event->type == TUI_EV_KEY &&
        (event->key == TUI_KEY_ENTER ||
         event->key == ' '))
        return checkbox_toggle(checkbox, control, event);

    return 0;
}

void tui_checkbox_init(TuiCheckBox *checkbox,
                      int x,
                      int y,
                      int width,
                      const char *text)
{
    tui_control_init(&checkbox->control,
                     &checkbox_class,
                     x, y,
                     width, 1,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    checkbox->text = text;
    checkbox->checked = 0;
    checkbox->command = TUI_CMD_NONE;
}

void tui_checkbox_set_checked(TuiCheckBox *checkbox, int checked)
{
    if (checkbox != 0)
        checkbox->checked = checked != 0;
}

int tui_checkbox_get_checked(TuiCheckBox *checkbox)
{
    return checkbox != 0 ? checkbox->checked : 0;
}

void tui_checkbox_set_command(TuiCheckBox *checkbox, int command)
{
    if (checkbox != 0)
        checkbox->command = command;
}
