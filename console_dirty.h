#ifndef TUI_CONSOLE_DIRTY_H
#define TUI_CONSOLE_DIRTY_H

#include <windows.h>

typedef int (*TuiDirtyRunFn)(int x, int y, int length,
                             const CHAR_INFO *cells,
                             void *context);

int tui_dirty_each_run(CHAR_INFO *current,
                       CHAR_INFO *previous,
                       int width,
                       int height,
                       int dirty,
                       TuiDirtyRunFn callback,
                       void *context);

#endif
