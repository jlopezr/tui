#include "console.h"

#define MINI_UART_DATA       (*(volatile unsigned int *)0x80100000)
#define MINI_UART_STATUS     (*(volatile unsigned int *)0x80100004)
#define MINI_VIDEO_COMMIT    (*(volatile unsigned int *)0x8020000c)
#define MINI_VIDEO_CONFIG    (*(volatile unsigned int *)0x80200040)
#define MINI_VIDEO_PALETTE   ((volatile unsigned int *)0x80201000)
#define MINI_TEXT_RAM        ((volatile unsigned int *)0x80206000)

#define MINI_WIDTH           80
#define MINI_HEIGHT          30
#define MINI_CELLS           (MINI_WIDTH * MINI_HEIGHT)
#define MINI_TEXT_ENABLE     4
#define MINI_STATE_COMMIT    2

#define MINI_FG_MASK         0x0f00
#define MINI_BG_MASK         0xf000

static int mini_cursor_x;
static int mini_cursor_y;
static int mini_cursor_visible;
static int mini_cursor_drawn;
static unsigned int mini_cursor_under;

/*
 * Hardware colour index 0 is transparent: a glyph pixel or background drawn
 * with it shows the framebuffer underneath. DOS black (0) therefore cannot
 * live there, and the 16 DOS colours have to share the 15 opaque slots.
 * Colours are laid out in DOS order from slot 1, skipping light magenta,
 * which is merged into magenta (the library uses neither). Yellow and white
 * keep their DOS numbers. Slot 0 is never written.
 */
static const unsigned char mini_slot[16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 6, 14, 15
};

static unsigned int mini_cell(int ch, int attr)
{
    return ((unsigned int)mini_slot[attr & 0x0f] << 8) |
           ((unsigned int)mini_slot[(attr >> 4) & 0x0f] << 12) |
           (unsigned int)(unsigned char)ch;
}

static int mini_glyph(int ch)
{
    switch (ch) {
    case TUI_CH_HLINE: return 0xc4;
    case TUI_CH_VLINE: return 0xb3;
    case TUI_CH_TL: return 0xda;
    case TUI_CH_TR: return 0xbf;
    case TUI_CH_BL: return 0xc0;
    case TUI_CH_BR: return 0xd9;
    case TUI_CH_DHLINE: return 0xcd;
    case TUI_CH_DVLINE: return 0xba;
    case TUI_CH_DTL: return 0xc9;
    case TUI_CH_DTR: return 0xbb;
    case TUI_CH_DBL: return 0xc8;
    case TUI_CH_DBR: return 0xbc;
    case TUI_CH_LTEE: return 0xc3;
    case TUI_CH_RTEE: return 0xb4;
    case TUI_CH_TTEE: return 0xc2;
    case TUI_CH_BTEE: return 0xc1;
    case TUI_CH_CROSS: return 0xc5;
    case TUI_CH_UP_TRIANGLE: return 0x1e;
    case TUI_CH_DOWN_TRIANGLE: return 0x1f;
    case TUI_CH_LEFT_TRIANGLE: return 0x11;
    case TUI_CH_RIGHT_TRIANGLE: return 0x10;
    case TUI_CH_SCROLL_TRACK: return 0xb0;
    case TUI_CH_SCROLL_THUMB: return 0xdb;
    case TUI_CH_CHECK: return 0xfb;
    case TUI_CH_BULLET: return 0x07;
    default: return (unsigned char)ch;
    }
}

static void mini_cursor_hide(void)
{
    if (mini_cursor_drawn) {
        MINI_TEXT_RAM[mini_cursor_y * MINI_WIDTH + mini_cursor_x] =
            mini_cursor_under;
        mini_cursor_drawn = 0;
    }
}

static void mini_cursor_show(void)
{
    if (!mini_cursor_visible)
        return;

    mini_cursor_under =
        MINI_TEXT_RAM[mini_cursor_y * MINI_WIDTH + mini_cursor_x];
    MINI_TEXT_RAM[mini_cursor_y * MINI_WIDTH + mini_cursor_x] =
        mini_cell('_', 0x0f);
    mini_cursor_drawn = 1;
}

static void mini_commit_video(int enabled)
{
    MINI_VIDEO_CONFIG = enabled ? MINI_TEXT_ENABLE : 0;
    MINI_VIDEO_COMMIT = MINI_STATE_COMMIT;
    while (MINI_VIDEO_COMMIT != 0)
        ;
}

static int mini_serial_available(void)
{
    return (int)(MINI_UART_STATUS & 0xff);
}

static int mini_serial_get(void)
{
    while (!mini_serial_available())
        ;
    return (int)(MINI_UART_DATA & 0xff);
}

static int mini_escape_key(void)
{
    int second;
    int third;

    if (!mini_serial_available())
        return TUI_KEY_ESCAPE;
    second = mini_serial_get();
    if (second != '[')
        return TUI_KEY_ESCAPE;
    if (!mini_serial_available())
        return TUI_KEY_ESCAPE;
    third = mini_serial_get();

    switch (third) {
    case 'A': return TUI_KEY_UP;
    case 'B': return TUI_KEY_DOWN;
    case 'C': return TUI_KEY_RIGHT;
    case 'D': return TUI_KEY_LEFT;
    case 'H': return TUI_KEY_HOME;
    case 'F': return TUI_KEY_END;
    default:
        return TUI_KEY_ESCAPE;
    }
}

int tui_console_init(void)
{
    static const unsigned int palette[16] = {
        0x00000000, 0x000000aa, 0x0000aa00, 0x0000aaaa,
        0x00aa0000, 0x00aa00aa, 0x00aa5500, 0x00aaaaaa,
        0x00555555, 0x005555ff, 0x0055ff55, 0x0055ffff,
        0x00ff5555, 0x00ff55ff, 0x00ffff55, 0x00ffffff
    };
    int i;

    /* Downwards, so magenta (5) is written after light magenta (13). */
    for (i = 15; i >= 0; --i)
        MINI_VIDEO_PALETTE[mini_slot[i]] = palette[i];
    for (i = 0; i < MINI_CELLS; ++i)
        MINI_TEXT_RAM[i] = mini_cell(' ', 0x07);

    mini_cursor_x = 0;
    mini_cursor_y = 0;
    mini_cursor_visible = 0;
    mini_cursor_drawn = 0;
    mini_commit_video(1);
    return 1;
}

void tui_console_shutdown(void)
{
    mini_cursor_hide();
    mini_commit_video(0);
}

int tui_console_width(void)
{
    return MINI_WIDTH;
}

int tui_console_height(void)
{
    return MINI_HEIGHT;
}

void tui_console_cell(int x, int y, int ch, int attr)
{
    if (x < 0 || x >= MINI_WIDTH || y < 0 || y >= MINI_HEIGHT)
        return;
    mini_cursor_hide();
    MINI_TEXT_RAM[y * MINI_WIDTH + x] =
        mini_cell(mini_glyph(ch), attr);
}

int tui_console_key(void)
{
    int key;

    key = mini_serial_get();
    switch (key) {
    case '\r':
    case '\n': return TUI_KEY_ENTER;
    case 27: return mini_escape_key();
    case 9: return TUI_KEY_TAB;
    case 8:
    case 127: return TUI_KEY_BACKSPACE;
    default: return key;
    }
}

void tui_console_mouse(int *x, int *y, int *action, int *buttons)
{
    *x = 0;
    *y = 0;
    *action = TUI_MOUSE_MOVE;
    *buttons = 0;
}

void tui_console_cursor(int x, int y, int visible)
{
    mini_cursor_hide();
    if (x < 0)
        x = 0;
    if (x >= MINI_WIDTH)
        x = MINI_WIDTH - 1;
    if (y < 0)
        y = 0;
    if (y >= MINI_HEIGHT)
        y = MINI_HEIGHT - 1;
    mini_cursor_x = x;
    mini_cursor_y = y;
    mini_cursor_visible = visible;
}

void tui_console_present(void)
{
    mini_cursor_hide();
    mini_cursor_show();
}
