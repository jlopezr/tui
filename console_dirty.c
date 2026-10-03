#include <windows.h>

#include "console_dirty.h"

int tui_dirty_each_run(CHAR_INFO *current,
                       CHAR_INFO *previous,
                       int width,
                       int height,
                       int dirty,
                       TuiDirtyRunFn callback,
                       void *context)
{
    int x;
    int y;
    int start;
    int end;
    int count;

    if (!dirty)
        return 1;

    for (y = 0; y < height; ++y) {
        x = 0;
        while (x < width) {
            while (x < width &&
                   current[y * width + x].Char.UnicodeChar ==
                   previous[y * width + x].Char.UnicodeChar &&
                   current[y * width + x].Attributes ==
                   previous[y * width + x].Attributes)
                ++x;
            start = x;
            while (x < width &&
                   (current[y * width + x].Char.UnicodeChar !=
                    previous[y * width + x].Char.UnicodeChar ||
                    current[y * width + x].Attributes !=
                    previous[y * width + x].Attributes))
                ++x;
            end = x;
            if (start != end &&
                !callback(start, y, end - start,
                          &current[y * width + start], context))
                return 0;
        }
    }

    count = width * height;
    CopyMemory(previous, current, (SIZE_T)count * sizeof(CHAR_INFO));
    return 1;
}
