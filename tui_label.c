#include "tui_internal.h"

static void label_draw(TuiControl *control, TuiDraw *draw);
static int label_event(TuiControl *control, TuiEvent *event);

static const TuiClass label_class = {
    label_draw,
    label_event
};

/*
 * ------------------------------------------------------------
 * Label
 * ------------------------------------------------------------
 */

static void label_draw(TuiControl *control, TuiDraw *draw)
{
    TuiLabel *label;

    label = (TuiLabel *)control;

    tui_text(draw,
             0, 0,
             label->text,
             tui_control_attr(control));
}

static int label_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;

    return 0;
}

void tui_label_init(TuiLabel *label,
                    int x, int y,
                    const char *text)
{
    tui_control_init(&label->control,
                     &label_class,
                     x, y,
                     tui_strlen(text), 1,
                     TUI_VISIBLE | TUI_ENABLED);

    label->text = text;
}

void tui_label_set_text(TuiLabel *label, const char *text)
{
    /* Old and new rectangle: the label may have got shorter. */
    tui_invalidate(&label->control);

    label->text = text;
    label->control.width = tui_strlen(text);

    tui_invalidate(&label->control);
}


