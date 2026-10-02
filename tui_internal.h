#ifndef TUI_INTERNAL_H
#define TUI_INTERNAL_H

#include "tui.h"

int tui_strlen(const char *s);
int tui_max(int a, int b);
int tui_min(int a, int b);

void tui_putc(TuiDraw *draw, int x, int y, int ch, int attr);
void tui_text(TuiDraw *draw, int x, int y,
              const char *text, int attr);
void tui_fill(TuiDraw *draw, int x, int y,
              int width, int height, int ch, int attr);
void tui_box(TuiDraw *draw, int width, int height, int attr);

void tui_control_init(TuiControl *control,
                      const TuiClass *cls,
                      int x, int y,
                      int width, int height,
                      int flags);

int tui_control_has_focus(TuiControl *control);
TuiDesktop *tui_find_desktop(TuiControl *control);
void tui_control_screen_pos(TuiControl *control,
                            int *screen_x, int *screen_y);

extern const TuiClass tui_window_class;

#endif
