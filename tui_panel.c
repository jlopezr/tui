#include "tui_internal.h"

static void panel_draw(TuiControl *control, TuiDraw *draw);
static int panel_event(TuiControl *control, TuiEvent *event);

static const TuiClass panel_class = {
    panel_draw,
    panel_event
};

/*
 * ------------------------------------------------------------
 * Panel
 *
 * A container with no frame, no title and no background: it only groups its
 * children and lays them out (docking works inside it like inside a window,
 * but without the one-cell border). Taking the whole panel in or out of the
 * tree takes all of them, which is what a "screen" of an application is.
 * ------------------------------------------------------------
 */

static void panel_draw(TuiControl *control, TuiDraw *draw)
{
    (void)control;
    (void)draw;
}

static int panel_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;

    return 0;
}

void tui_panel_init(TuiPanel *panel,
                    int x, int y,
                    int width, int height)
{
    tui_control_init(&panel->control,
                     &panel_class,
                     x, y,
                     width, height,
                     TUI_VISIBLE | TUI_ENABLED);
}
