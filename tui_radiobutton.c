#include "tui_internal.h"

static void radiobutton_draw(TuiControl *control, TuiDraw *draw);
static int radiobutton_event(TuiControl *control, TuiEvent *event);

static const TuiClass radiobutton_class = {
    radiobutton_draw,
    radiobutton_event,
    0,
    TUI_CLASS_OPAQUE
};

static int radiobutton_attr(TuiControl *control)
{
    if (tui_control_has_focus(control))
        return TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);

    return TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);
}

static void radiobutton_draw(TuiControl *control, TuiDraw *draw)
{
    TuiRadioButton *radio;
    int attr;
    int x;
    const char *text;

    radio = (TuiRadioButton *)control;
    attr = radiobutton_attr(control);

    tui_fill(draw, 0, 0, control->width, 1, ' ', attr);

    if (control->width <= 0)
        return;

    tui_putc(draw, 0, 0, '(', attr);

    if (control->width > 1)
        tui_putc(draw, 1, 0, radio->checked ? TUI_CH_BULLET : ' ', attr);

    if (control->width > 2)
        tui_putc(draw, 2, 0, ')', attr);

    text = radio->text;
    x = 4;

    while (text != 0 &&
           *text != '\0' &&
           x < control->width) {
        tui_putc(draw, x, 0, (unsigned char)*text, attr);
        ++text;
        ++x;
    }
}

static int radiobutton_select(TuiRadioButton *radio,
                              TuiControl *control,
                              TuiEvent *event)
{
    TuiControl *sibling;
    int changed;

    changed = !radio->checked;

    if (control->parent != 0) {
        sibling = control->parent->first;

        while (sibling != 0) {
            if (sibling != control &&
                sibling->cls == &radiobutton_class) {
                TuiRadioButton *other;

                other = (TuiRadioButton *)sibling;

                if (other->group == radio->group && other->checked) {
                    other->checked = 0;
                    tui_invalidate(sibling);
                }
            }

            sibling = sibling->next;
        }
    }

    radio->checked = 1;
    tui_invalidate(control);

    if (!changed || radio->command == TUI_CMD_NONE)
        return 1;

    event->type = TUI_EV_COMMAND;
    event->command = radio->command;
    event->source = control;

    return 1;
}

static int radiobutton_event(TuiControl *control, TuiEvent *event)
{
    TuiRadioButton *radio;

    radio = (TuiRadioButton *)control;

    if (event->type == TUI_EV_MOUSE &&
        event->mouse_action == TUI_MOUSE_DOWN &&
        (event->mouse_buttons & TUI_MOUSE_LEFT))
        return radiobutton_select(radio, control, event);

    if (event->type == TUI_EV_KEY &&
        (event->key == TUI_KEY_ENTER ||
         event->key == ' '))
        return radiobutton_select(radio, control, event);

    return 0;
}

void tui_radiobutton_init(TuiRadioButton *radio,
                         int x,
                         int y,
                         int width,
                         const char *text,
                         int group)
{
    tui_control_init(&radio->control,
                     &radiobutton_class,
                     x, y,
                     width, 1,
                     TUI_VISIBLE |
                     TUI_ENABLED |
                     TUI_FOCUSABLE |
                     TUI_TABSTOP);

    radio->text = text;
    radio->group = group;
    radio->checked = 0;
    radio->command = TUI_CMD_NONE;
}

void tui_radiobutton_set_checked(TuiRadioButton *radio, int checked)
{
    TuiControl *sibling;

    if (radio == 0)
        return;

    if (!checked) {
        radio->checked = 0;
        tui_invalidate(&radio->control);
        return;
    }

    if (radio->control.parent != 0) {
        sibling = radio->control.parent->first;

        while (sibling != 0) {
            if (sibling != &radio->control &&
                sibling->cls == &radiobutton_class) {
                TuiRadioButton *other;

                other = (TuiRadioButton *)sibling;

                if (other->group == radio->group && other->checked) {
                    other->checked = 0;
                    tui_invalidate(sibling);
                }
            }

            sibling = sibling->next;
        }
    }

    radio->checked = 1;
    tui_invalidate(&radio->control);
}

int tui_radiobutton_get_checked(TuiRadioButton *radio)
{
    return radio != 0 ? radio->checked : 0;
}

void tui_radiobutton_set_command(TuiRadioButton *radio, int command)
{
    if (radio != 0)
        radio->command = command;
}
