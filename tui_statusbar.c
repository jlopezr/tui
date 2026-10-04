#include "tui_internal.h"

static void statusbar_draw(TuiControl *control, TuiDraw *draw);
static int statusbar_event(TuiControl *control, TuiEvent *event);

static const TuiClass statusbar_class = {
    statusbar_draw,
    statusbar_event
};

/*
 * ------------------------------------------------------------
 * Status bar
 * ------------------------------------------------------------
 */

/*
 * Text for the key hint, e.g. "F10 ". Returns its length
 * (0 when the item has no function key).
 */
static int tui_statusitem_keyname(TuiStatusItem *item, char *buf)
{
    int fn;
    int n;

    if (item->key < TUI_KEY_F1 || item->key > TUI_KEY_F12) {
        buf[0] = 0;
        return 0;
    }

    fn = item->key - TUI_KEY_F1 + 1;
    n = 0;

    buf[n++] = 'F';

    if (fn >= 10)
        buf[n++] = (char)('0' + (fn / 10));

    buf[n++] = (char)('0' + (fn % 10));
    buf[n++] = ' ';
    buf[n] = 0;

    return n;
}

static int tui_statusitem_width(TuiStatusItem *item)
{
    char key[8];

    return tui_statusitem_keyname(item, key) +
           (item->text != 0 ? tui_strlen(item->text) : 0);
}

/* Items start at x = 1 and are separated by 2 columns. */
static int tui_statusitem_x(TuiStatusBar *bar, int index)
{
    int i;
    int x;

    x = 1;

    for (i = 0; i < index; ++i)
        x += tui_statusitem_width(&bar->items[i]) + 2;

    return x;
}

static void statusbar_draw(TuiControl *control, TuiDraw *draw)
{
    TuiStatusBar *bar;
    TuiStatusItem *item;
    int attr;
    int x;
    int i;
    int len;
    int status_len;
    int status_x;
    char key[8];

    bar = (TuiStatusBar *)control;

    attr = TUI_ATTR_STATUSBAR;

    /*
     * Fill complete status bar.
     */
    tui_fill(draw,
             0, 0,
             control->width, 1,
             ' ',
             attr);

    /*
     * Draw command hints from left to right.
     *
     * For now the text contains only the description.
     * The key name is generated separately.
     */
    x = 1;

    for (i = 0; i < bar->count; ++i) {
        item = &bar->items[i];

        x = tui_statusitem_x(bar, i);

        /*
         * Draw function key.
         */
        len = tui_statusitem_keyname(item, key);

        if (len > 0) {
            tui_text(draw, x, 0, key, attr);
            x += len;
        }

        /*
         * Description.
         */
        if (item->text != 0) {
            len = tui_strlen(item->text);

            tui_text(draw,
                     x, 0,
                     item->text,
                     attr);

            x += len;
        }
    }

    /*
     * Status text aligned to the right.
     */
    if (bar->status != 0) {
        status_len = tui_strlen(bar->status);

        status_x =
            control->width - status_len - 1;

        /*
         * Do not overwrite the left-side items.
         */
        if (status_x > x) {
            tui_text(draw,
                     status_x, 0,
                     bar->status,
                     attr);
        }
    }
}


static int statusbar_event(TuiControl *control,
                           TuiEvent *event)
{
    TuiStatusBar *bar;
    int lx;
    int ly;
    int i;
    int x0;

    if (event->type != TUI_EV_MOUSE ||
        event->mouse_action != TUI_MOUSE_DOWN ||
        !(event->mouse_buttons & TUI_MOUSE_LEFT))
        return 0;

    bar = (TuiStatusBar *)control;

    tui_control_screen_to_local(control,
                                event->mouse_x,
                                event->mouse_y,
                                &lx, &ly);

    for (i = 0; i < bar->count; ++i) {
        x0 = tui_statusitem_x(bar, i);

        if (lx >= x0 &&
            lx < x0 + tui_statusitem_width(&bar->items[i])) {

            event->type = TUI_EV_COMMAND;
            event->command = bar->items[i].command;
            event->source = control;

            return 1;
        }
    }

    return 0;
}


void tui_statusbar_init(TuiStatusBar *bar,
                        TuiStatusItem *items,
                        int count)
{
    tui_control_init(
        &bar->control,
        &statusbar_class,
        0, 0,
        0, 1,
        TUI_VISIBLE | TUI_ENABLED);

    bar->control.dock = TUI_DOCK_BOTTOM;

    bar->items = items;
    bar->count = count;
    bar->status = 0;
}


static int statusbar_same_text(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }

    return *a == *b;
}

void tui_statusbar_set_text(TuiStatusBar *bar,
                            const char *text)
{
    /*
     * A different pointer with the same text changes nothing on screen. The
     * same pointer may be a buffer the caller edited in place: draw it again.
     */
    if (bar->status != text && bar->status != 0 && text != 0 &&
        statusbar_same_text(bar->status, text))
        return;

    bar->status = text;
    tui_invalidate(&bar->control);
}

