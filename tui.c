#include "tui.h"

static int tui_strlen(const char *s)
{
    int n;

    n = 0;
    while (s != 0 && *s != 0) {
        ++n;
        ++s;
    }
    return n;
}

static int tui_max(int a, int b)
{
    return a > b ? a : b;
}

static int tui_min(int a, int b)
{
    return a < b ? a : b;
}

static void tui_putc(TuiDraw *d, int x, int y, int ch, int attr)
{
    int sx;
    int sy;

    sx = d->ox + x;
    sy = d->oy + y;

    if (sx < d->x1 || sx >= d->x2 ||
        sy < d->y1 || sy >= d->y2)
        return;

    tui_console_cell(sx, sy, ch, attr);
}

static void tui_text(TuiDraw *d, int x, int y,
                     const char *s, int attr)
{
    while (s != 0 && *s != 0) {
        tui_putc(d, x, y, (unsigned char)*s, attr);
        ++x;
        ++s;
    }
}

static void tui_fill(TuiDraw *d, int x, int y,
                     int w, int h, int ch, int attr)
{
    int xx;
    int yy;

    for (yy = 0; yy < h; ++yy)
        for (xx = 0; xx < w; ++xx)
            tui_putc(d, x + xx, y + yy, ch, attr);
}

static void tui_box(TuiDraw *d, int w, int h, int attr)
{
    int x;
    int y;

    if (w < 2 || h < 2)
        return;

    tui_putc(d, 0, 0, '+', attr);
    tui_putc(d, w - 1, 0, '+', attr);
    tui_putc(d, 0, h - 1, '+', attr);
    tui_putc(d, w - 1, h - 1, '+', attr);

    for (x = 1; x < w - 1; ++x) {
        tui_putc(d, x, 0, '-', attr);
        tui_putc(d, x, h - 1, '-', attr);
    }

    for (y = 1; y < h - 1; ++y) {
        tui_putc(d, 0, y, '|', attr);
        tui_putc(d, w - 1, y, '|', attr);
    }
}

static void tui_control_base_init(TuiControl *c,
                                  const TuiClass *cls,
                                  int x, int y,
                                  int w, int h,
                                  int flags)
{
    c->cls = cls;
    c->parent = 0;
    c->first = 0;
    c->last = 0;
    c->next = 0;
    c->prev = 0;
    c->x = x;
    c->y = y;
    c->width = w;
    c->height = h;
    c->flags = flags;
}

static void tui_child_context(TuiControl *parent,
                              TuiDraw *parent_draw,
                              TuiDraw *child_draw)
{
    int left;
    int top;
    int right;
    int bottom;

    *child_draw = *parent_draw;

    /*
     * Window children use the window client area.
     * For this first version every container is a window.
     */
    left = parent_draw->ox + 1;
    top = parent_draw->oy + 1;
    right = parent_draw->ox + parent->width - 1;
    bottom = parent_draw->oy + parent->height - 1;

    child_draw->x1 = tui_max(child_draw->x1, left);
    child_draw->y1 = tui_max(child_draw->y1, top);
    child_draw->x2 = tui_min(child_draw->x2, right);
    child_draw->y2 = tui_min(child_draw->y2, bottom);

    child_draw->ox = left;
    child_draw->oy = top;
}

static void tui_draw_tree(TuiControl *control, TuiDraw *draw)
{
    TuiControl *child;
    TuiDraw client;
    TuiDraw cd;

    if ((control->flags & TUI_VISIBLE) == 0)
        return;

    if (control->cls != 0 && control->cls->draw != 0)
        control->cls->draw(control, draw);

    if (control->first == 0)
        return;

    tui_child_context(control, draw, &client);

    child = control->first;
    while (child != 0) {
        cd = client;
        cd.ox += child->x;
        cd.oy += child->y;
        tui_draw_tree(child, &cd);
        child = child->next;
    }
}

static void window_draw(TuiControl *control, TuiDraw *d)
{
    TuiWindow *window;
    int attr;
    int title_len;
    int title_x;

    window = (TuiWindow *)control;
    attr = TUI_ATTR(TUI_WHITE, TUI_BLUE);

    tui_fill(d, 0, 0, control->width, control->height, ' ', attr);
    tui_box(d, control->width, control->height, attr);

    if (window->title != 0) {
        title_len = tui_strlen(window->title);
        title_x = (control->width - title_len - 2) / 2;
        if (title_x < 1)
            title_x = 1;
        tui_putc(d, title_x, 0, ' ', attr);
        tui_text(d, title_x + 1, 0, window->title, attr);
        tui_putc(d, title_x + title_len + 1, 0, ' ', attr);
    }
}

static int window_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;
    return 0;
}

static void label_draw(TuiControl *control, TuiDraw *d)
{
    TuiLabel *label;

    label = (TuiLabel *)control;
    tui_text(d, 0, 0, label->text,
             TUI_ATTR(TUI_LIGHTGRAY, TUI_BLUE));
}

static int label_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    (void)event;
    return 0;
}

static int tui_is_focused(TuiControl *control)
{
    TuiWindow *window;

    if (control->parent == 0)
        return 0;

    window = (TuiWindow *)control->parent;
    return window->focused == control;
}

static void button_draw(TuiControl *control, TuiDraw *d)
{
    TuiButton *button;
    int attr;
    int len;
    int x;

    button = (TuiButton *)control;

    if (tui_is_focused(control))
        attr = TUI_ATTR(TUI_BLUE, TUI_LIGHTGRAY);
    else
        attr = TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);

    tui_fill(d, 0, 0, control->width, 1, ' ', attr);

    len = tui_strlen(button->text);
    x = (control->width - len - 2) / 2;
    if (x < 0)
        x = 0;

    tui_putc(d, x, 0, '[', attr);
    tui_text(d, x + 1, 0, button->text, attr);
    tui_putc(d, x + len + 1, 0, ']', attr);
}

static int button_event(TuiControl *control, TuiEvent *event)
{
    TuiButton *button;

    button = (TuiButton *)control;

    if (event->type == TUI_EV_KEY &&
        event->key == TUI_KEY_ENTER) {
        event->type = TUI_EV_COMMAND;
        event->command = button->command;
        event->source = control;
        return 1;
    }

    return 0;
}

static const TuiClass window_class = {
    window_draw,
    window_event
};

static const TuiClass label_class = {
    label_draw,
    label_event
};

static const TuiClass button_class = {
    button_draw,
    button_event
};

int tui_init(void)
{
    return tui_console_init();
}

void tui_shutdown(void)
{
    tui_console_shutdown();
}

void tui_add(TuiControl *parent, TuiControl *child)
{
    child->parent = parent;
    child->prev = parent->last;
    child->next = 0;

    if (parent->last != 0)
        parent->last->next = child;
    else
        parent->first = child;

    parent->last = child;
}

void tui_draw(TuiControl *root)
{
    TuiDraw d;
    int w;
    int h;

    w = tui_console_width();
    h = tui_console_height();

    d.ox = root->x;
    d.oy = root->y;
    d.x1 = 0;
    d.y1 = 0;
    d.x2 = w;
    d.y2 = h;

    tui_draw_tree(root, &d);
    tui_console_present();
}

static TuiControl *tui_first_focusable(TuiWindow *window)
{
    TuiControl *c;

    c = window->control.first;
    while (c != 0) {
        if ((c->flags & (TUI_VISIBLE | TUI_ENABLED |
                         TUI_FOCUSABLE | TUI_TABSTOP)) ==
            (TUI_VISIBLE | TUI_ENABLED |
             TUI_FOCUSABLE | TUI_TABSTOP))
            return c;
        c = c->next;
    }

    return 0;
}

static TuiControl *tui_next_focusable(TuiWindow *window,
                                      TuiControl *from)
{
    TuiControl *c;

    if (from == 0)
        return tui_first_focusable(window);

    c = from->next;
    if (c == 0)
        c = window->control.first;

    while (c != from) {
        if ((c->flags & (TUI_VISIBLE | TUI_ENABLED |
                         TUI_FOCUSABLE | TUI_TABSTOP)) ==
            (TUI_VISIBLE | TUI_ENABLED |
             TUI_FOCUSABLE | TUI_TABSTOP))
            return c;

        c = c->next;
        if (c == 0)
            c = window->control.first;
    }

    return from;
}

int tui_dispatch(TuiWindow *window, TuiEvent *event)
{
    if (event->type != TUI_EV_KEY)
        return 0;

    if (event->key == TUI_KEY_TAB) {
        window->focused =
            tui_next_focusable(window, window->focused);
        return 1;
    }

    if (window->focused != 0 &&
        window->focused->cls != 0 &&
        window->focused->cls->event != 0) {
        if (window->focused->cls->event(window->focused, event))
            return 1;
    }

    if (window->control.cls != 0 &&
        window->control.cls->event != 0)
        return window->control.cls->event(&window->control, event);

    return 0;
}

int tui_read_event(TuiEvent *event)
{
    int key;

    key = tui_console_key();

    event->type = TUI_EV_KEY;
    event->key = key;
    event->command = TUI_CMD_NONE;
    event->source = 0;

    return 1;
}

void tui_window_init(TuiWindow *window,
                     int x, int y, int width, int height,
                     const char *title)
{
    tui_control_base_init(&window->control, &window_class,
                          x, y, width, height,
                          TUI_VISIBLE | TUI_ENABLED);
    window->title = title;
    window->focused = 0;
}

void tui_label_init(TuiLabel *label,
                    int x, int y,
                    const char *text)
{
    tui_control_base_init(&label->control, &label_class,
                          x, y, tui_strlen(text), 1,
                          TUI_VISIBLE | TUI_ENABLED);
    label->text = text;
}

void tui_button_init(TuiButton *button,
                     int x, int y, int width,
                     const char *text,
                     int command)
{
    tui_control_base_init(&button->control, &button_class,
                          x, y, width, 1,
                          TUI_VISIBLE | TUI_ENABLED |
                          TUI_FOCUSABLE | TUI_TABSTOP);
    button->text = text;
    button->command = command;
}

void tui_label_set_text(TuiLabel *label, const char *text)
{
    label->text = text;
    label->control.width = tui_strlen(text);
}
