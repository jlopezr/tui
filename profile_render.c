#include <stdio.h>
#include <windows.h>

#include "console.h"

#define PROFILE_FRAMES 1000

static void profile_full_frame(int width, int height, int attr)
{
    int x;
    int y;

    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
            tui_console_cell(x, y, ' ', attr);
}

static void profile_text_frame(int width, int height, int frame)
{
    int x;
    int y;

    y = height / 2;
    for (x = 0; x < width; ++x)
        tui_console_cell(x, y, ' ', 0x07);
    x = frame % width;
    tui_console_cell(x, y, 'X', 0x0f);
    tui_console_cursor(x, y, 1);
}

int main(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER finish;
    int width;
    int height;
    int frame;
    double elapsed;

    if (!tui_console_init())
        return 1;

    width = tui_console_width();
    height = tui_console_height();
    if (width <= 0 || height <= 0) {
        tui_console_shutdown();
        return 1;
    }

    profile_full_frame(width, height, 0x07);
    tui_console_present();
    profile_full_frame(width, height, 0x07);

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    for (frame = 0; frame < PROFILE_FRAMES; ++frame) {
        profile_text_frame(width, height, frame);
        tui_console_present();
    }
    QueryPerformanceCounter(&finish);

    elapsed = (double)(finish.QuadPart - start.QuadPart) /
              (double)frequency.QuadPart;
    tui_console_shutdown();

    printf("render benchmark\n");
    printf("size: %dx%d\n", width, height);
    printf("frames: %d\n", PROFILE_FRAMES);
    printf("elapsed: %.3f ms\n", elapsed * 1000.0);
    printf("average frame: %.3f ms\n",
           elapsed * 1000.0 / (double)PROFILE_FRAMES);
    return 0;
}
