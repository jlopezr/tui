#include <windows.h>
#include "console.h"
#include "console_dirty.h"

#define TUI_MOUSE_DOUBLE_CLICK_MS 500

static HANDLE tui_input;
static HANDLE tui_output;
static DWORD tui_input_mode;
static DWORD tui_output_mode;
static int tui_console_ready;
static CHAR_INFO *tui_frame;
static CHAR_INFO *tui_previous_frame;
static int tui_frame_width;
static int tui_frame_height;
static int tui_frame_dirty;
static int tui_window_width;
static int tui_window_height;

static int tui_mouse_x;
static int tui_mouse_y;
static int tui_mouse_action;
static int tui_mouse_buttons;
static int tui_mouse_left_pressed;
static int tui_mouse_left_moved;
static int tui_mouse_press_x;
static int tui_mouse_press_y;
static DWORD tui_mouse_press_time;
static DWORD tui_mouse_previous_buttons;
static int tui_mouse_last_click_valid;
static int tui_mouse_last_click_x;
static int tui_mouse_last_click_y;
static DWORD tui_mouse_last_click_time;

static WORD tui_win32_attr(int attr)
{
    return (WORD)((attr & 0x0f) | ((attr >> 4) & 0x0f) << 4);
}

static int tui_win32_position(int x, int y, COORD *pos)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(tui_output, &info))
        return 0;

    pos->X = (SHORT)(x + info.srWindow.Left);
    pos->Y = (SHORT)(y + info.srWindow.Top);
    return 1;
}

static int tui_win32_frame_size(int width, int height)
{
    CHAR_INFO *frame;
    CHAR_INFO *previous;
    int count;
    int i;

    if (width <= 0 || height <= 0)
        return 0;

    if (width == tui_frame_width && height == tui_frame_height &&
        tui_frame != 0)
        return 1;

    count = width * height;
    frame = (CHAR_INFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   (SIZE_T)count * sizeof(CHAR_INFO));
    previous = (CHAR_INFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      (SIZE_T)count * sizeof(CHAR_INFO));
    if (frame == 0 || previous == 0) {
        if (frame != 0)
            HeapFree(GetProcessHeap(), 0, frame);
        if (previous != 0)
            HeapFree(GetProcessHeap(), 0, previous);
        return 0;
    }

    for (i = 0; i < count; ++i) {
        frame[i].Char.UnicodeChar = L' ';
        frame[i].Attributes = tui_win32_attr(0x07);
        previous[i].Char.UnicodeChar = 0;
        previous[i].Attributes = 0;
    }

    if (tui_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_frame);
    if (tui_previous_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_previous_frame);
    tui_frame = frame;
    tui_previous_frame = previous;
    tui_frame_width = width;
    tui_frame_height = height;
    tui_frame_dirty = 1;
    return 1;
}

static WCHAR tui_win32_glyph(int ch)
{
    switch (ch) {
    case TUI_CH_HLINE:
        return 0x2500;
    case TUI_CH_VLINE:
        return 0x2502;
    case TUI_CH_TL:
        return 0x250c;
    case TUI_CH_TR:
        return 0x2510;
    case TUI_CH_BL:
        return 0x2514;
    case TUI_CH_BR:
        return 0x2518;
    case TUI_CH_DHLINE:
        return 0x2550;
    case TUI_CH_DVLINE:
        return 0x2551;
    case TUI_CH_DTL:
        return 0x2554;
    case TUI_CH_DTR:
        return 0x2557;
    case TUI_CH_DBL:
        return 0x255a;
    case TUI_CH_DBR:
        return 0x255d;
    case TUI_CH_LTEE:
        return 0x251c;
    case TUI_CH_RTEE:
        return 0x2524;
    case TUI_CH_TTEE:
        return 0x252c;
    case TUI_CH_BTEE:
        return 0x2534;
    case TUI_CH_CROSS:
        return 0x253c;
    case TUI_CH_UP_TRIANGLE:
        return 0x25b2;
    case TUI_CH_DOWN_TRIANGLE:
        return 0x25bc;
    case TUI_CH_LEFT_TRIANGLE:
        return 0x25c0;
    case TUI_CH_RIGHT_TRIANGLE:
        return 0x25b6;
    case TUI_CH_SCROLL_TRACK:
        return 0x2592;
    case TUI_CH_SCROLL_THUMB:
        return 0x2588;
    case TUI_CH_CHECK:
        return 0x221a;
    case TUI_CH_BULLET:
        return 0x2022;
    default:
        return (WCHAR)(unsigned char)ch;
    }
}

static int tui_win32_double_click(int x, int y)
{
    DWORD now;

    if (!tui_mouse_last_click_valid ||
        x != tui_mouse_last_click_x ||
        y != tui_mouse_last_click_y)
        return 0;

    now = GetTickCount();
    tui_mouse_last_click_valid = 0;

    return now - tui_mouse_last_click_time <=
           TUI_MOUSE_DOUBLE_CLICK_MS;
}

static int tui_win32_mouse_event(const MOUSE_EVENT_RECORD *ev)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    int changed;
    int left_down;
    int right_down;
    int middle_down;
    DWORD now;

    tui_mouse_x = ev->dwMousePosition.X;
    tui_mouse_y = ev->dwMousePosition.Y;
    if (GetConsoleScreenBufferInfo(tui_output, &info)) {
        tui_mouse_x -= info.srWindow.Left;
        tui_mouse_y -= info.srWindow.Top;
    }
    tui_mouse_action = TUI_MOUSE_MOVE;
    tui_mouse_buttons = 0;
    changed = 0;

    left_down = (ev->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
    right_down = (ev->dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;
    middle_down = (ev->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) != 0;

    if (ev->dwEventFlags == DOUBLE_CLICK && left_down) {
        tui_mouse_action = TUI_MOUSE_DOUBLE;
        changed = TUI_MOUSE_LEFT;
        tui_mouse_left_pressed = 1;
        tui_mouse_left_moved = 0;
    } else if (ev->dwEventFlags == 0 && left_down &&
               !tui_mouse_left_pressed) {
        tui_mouse_action = tui_win32_double_click(
            tui_mouse_x, tui_mouse_y) ? TUI_MOUSE_DOUBLE : TUI_MOUSE_DOWN;
        changed = TUI_MOUSE_LEFT;
        tui_mouse_left_pressed = 1;
        tui_mouse_left_moved = 0;
        tui_mouse_press_x = tui_mouse_x;
        tui_mouse_press_y = tui_mouse_y;
        tui_mouse_press_time = GetTickCount();
    } else if (ev->dwEventFlags == 0 && !left_down &&
               tui_mouse_left_pressed) {
        tui_mouse_action = TUI_MOUSE_UP;
        changed = TUI_MOUSE_LEFT;
        if (!tui_mouse_left_moved &&
            tui_mouse_x == tui_mouse_press_x &&
            tui_mouse_y == tui_mouse_press_y &&
            GetTickCount() - tui_mouse_press_time <=
                TUI_MOUSE_DOUBLE_CLICK_MS) {
            now = GetTickCount();
            tui_mouse_last_click_valid = 1;
            tui_mouse_last_click_x = tui_mouse_x;
            tui_mouse_last_click_y = tui_mouse_y;
            tui_mouse_last_click_time = now;
        } else {
            tui_mouse_last_click_valid = 0;
        }
        tui_mouse_left_pressed = 0;
    } else if (ev->dwEventFlags == MOUSE_MOVED) {
        if (tui_mouse_left_pressed &&
            (tui_mouse_x != tui_mouse_press_x ||
             tui_mouse_y != tui_mouse_press_y))
            tui_mouse_left_moved = 1;
        if (left_down)
            changed = TUI_MOUSE_LEFT;
        else if (right_down)
            changed = TUI_MOUSE_RIGHT;
        else if (middle_down)
            changed = TUI_MOUSE_MIDDLE;
    } else if (right_down ||
               (tui_mouse_previous_buttons &
                RIGHTMOST_BUTTON_PRESSED) != 0) {
        tui_mouse_action = right_down ? TUI_MOUSE_DOWN : TUI_MOUSE_UP;
        changed = TUI_MOUSE_RIGHT;
    } else if (middle_down ||
               (tui_mouse_previous_buttons &
                FROM_LEFT_2ND_BUTTON_PRESSED) != 0) {
        tui_mouse_action = middle_down ? TUI_MOUSE_DOWN : TUI_MOUSE_UP;
        changed = TUI_MOUSE_MIDDLE;
    }

    tui_mouse_buttons = changed;
    tui_mouse_previous_buttons = ev->dwButtonState;
    return 1;
}

static int tui_win32_key(const KEY_EVENT_RECORD *ev)
{
    switch (ev->wVirtualKeyCode) {
    case VK_RETURN: return TUI_KEY_ENTER;
    case VK_ESCAPE: return TUI_KEY_ESCAPE;
    case VK_TAB:
        return (ev->dwControlKeyState & SHIFT_PRESSED)
            ? TUI_KEY_BACKTAB : TUI_KEY_TAB;
    case VK_UP: return TUI_KEY_UP;
    case VK_DOWN: return TUI_KEY_DOWN;
    case VK_LEFT: return TUI_KEY_LEFT;
    case VK_RIGHT: return TUI_KEY_RIGHT;
    case VK_HOME: return TUI_KEY_HOME;
    case VK_END: return TUI_KEY_END;
    case VK_PRIOR: return TUI_KEY_PAGEUP;
    case VK_NEXT: return TUI_KEY_PAGEDOWN;
    case VK_DELETE: return TUI_KEY_DELETE;
    case VK_INSERT: return TUI_KEY_INSERT;
    case VK_BACK: return TUI_KEY_BACKSPACE;
    case VK_F1: return TUI_KEY_F1;
    case VK_F2: return TUI_KEY_F2;
    case VK_F3: return TUI_KEY_F3;
    case VK_F4: return TUI_KEY_F4;
    case VK_F5: return TUI_KEY_F5;
    case VK_F6: return TUI_KEY_F6;
    case VK_F7: return TUI_KEY_F7;
    case VK_F8: return TUI_KEY_F8;
    case VK_F9: return TUI_KEY_F9;
    case VK_F10: return TUI_KEY_F10;
    case VK_F11: return TUI_KEY_F11;
    case VK_F12: return TUI_KEY_F12;
    default:
        if (ev->uChar.UnicodeChar != 0)
            return (int)ev->uChar.UnicodeChar;
        return TUI_KEY_NONE;
    }
}

int tui_console_init(void)
{
    CONSOLE_CURSOR_INFO cursor;
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD origin;
    DWORD cells;
    DWORD written;
    WCHAR blank;
    WORD attribute;

    tui_input = GetStdHandle(STD_INPUT_HANDLE);
    tui_output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (tui_input == INVALID_HANDLE_VALUE ||
        tui_output == INVALID_HANDLE_VALUE)
        return 0;

    if (!GetConsoleMode(tui_input, &tui_input_mode) ||
        !GetConsoleMode(tui_output, &tui_output_mode))
        return 0;

    if (!SetConsoleMode(tui_input, ENABLE_EXTENDED_FLAGS |
                        ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
        return 0;
    if (!SetConsoleMode(tui_output, tui_output_mode |
                        ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)) {
        SetConsoleMode(tui_input, tui_input_mode);
        return 0;
    }

    cursor.dwSize = 25;
    cursor.bVisible = FALSE;
    SetConsoleCursorInfo(tui_output, &cursor);

    if (GetConsoleScreenBufferInfo(tui_output, &info)) {
        origin.X = 0;
        origin.Y = 0;
        cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
        blank = L' ';
        attribute = tui_win32_attr(0x07);
        FillConsoleOutputCharacterW(tui_output, blank, cells,
                                     origin, &written);
        FillConsoleOutputAttribute(tui_output, attribute, cells,
                                    origin, &written);
    }

    tui_mouse_left_pressed = 0;
    tui_mouse_left_moved = 0;
    tui_mouse_previous_buttons = 0;
    tui_mouse_last_click_valid = 0;
    tui_frame = 0;
    tui_previous_frame = 0;
    tui_frame_width = 0;
    tui_frame_height = 0;
    tui_frame_dirty = 0;
    tui_console_ready = 1;
    return 1;
}

void tui_console_shutdown(void)
{
    CONSOLE_CURSOR_INFO cursor;
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD origin;
    DWORD cells;
    DWORD written;
    WCHAR blank;
    WORD attribute;

    if (!tui_console_ready)
        return;

    if (GetConsoleScreenBufferInfo(tui_output, &info)) {
        origin = info.dwCursorPosition;
        origin.X = info.srWindow.Left;
        origin.Y = info.srWindow.Top;
        cells = (DWORD)(info.srWindow.Right - info.srWindow.Left + 1) *
                (DWORD)(info.srWindow.Bottom - info.srWindow.Top + 1);
        blank = L' ';
        attribute = tui_win32_attr(0x07);
        FillConsoleOutputCharacterW(tui_output, blank, cells,
                                     origin, &written);
        FillConsoleOutputAttribute(tui_output, attribute, cells,
                                    origin, &written);
        SetConsoleCursorPosition(tui_output, origin);
    }

    SetConsoleMode(tui_input, tui_input_mode);
    SetConsoleMode(tui_output, tui_output_mode);
    cursor.dwSize = 25;
    cursor.bVisible = TRUE;
    SetConsoleCursorInfo(tui_output, &cursor);
    if (tui_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_frame);
    if (tui_previous_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_previous_frame);
    tui_frame = 0;
    tui_previous_frame = 0;
    tui_frame_width = 0;
    tui_frame_height = 0;
    tui_console_ready = 0;
}

int tui_console_width(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(tui_output, &info))
        return 0;
    tui_window_width = info.srWindow.Right - info.srWindow.Left + 1;
    return tui_window_width;
}

int tui_console_height(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(tui_output, &info))
        return 0;
    tui_window_height = info.srWindow.Bottom - info.srWindow.Top + 1;
    return tui_window_height;
}

void tui_console_cell(int x, int y, int ch, int attr)
{
    int width;
    int height;
    WCHAR glyph;

    width = tui_window_width;
    height = tui_window_height;
    if (!tui_win32_frame_size(width, height) ||
        x < 0 || x >= width || y < 0 || y >= height)
        return;
    glyph = tui_win32_glyph(ch);
    tui_frame[y * width + x].Char.UnicodeChar = glyph;
    tui_frame[y * width + x].Attributes = tui_win32_attr(attr);
    tui_frame_dirty = 1;
}

int tui_console_key(void)
{
    INPUT_RECORD record;
    DWORD read;

    for (;;) {
        if (!ReadConsoleInputW(tui_input, &record, 1, &read) || read == 0)
            return TUI_KEY_NONE;

        if (record.EventType == KEY_EVENT &&
            record.Event.KeyEvent.bKeyDown)
            return tui_win32_key(&record.Event.KeyEvent);

        if (record.EventType == MOUSE_EVENT &&
            tui_win32_mouse_event(&record.Event.MouseEvent))
            return TUI_KEY_MOUSE;
    }
}

void tui_console_mouse(int *x, int *y, int *action, int *buttons)
{
    *x = tui_mouse_x;
    *y = tui_mouse_y;
    *action = tui_mouse_action;
    *buttons = tui_mouse_buttons;
}

int tui_console_has_mouse(void)
{
    return 1;               /* ENABLE_MOUSE_INPUT en tui_console_init */
}

int tui_console_printable(int key)
{
    return TUI_ASCII_PRINTABLE(key);
}

void tui_console_cursor(int x, int y, int visible)
{
    CONSOLE_CURSOR_INFO cursor;
    COORD pos;

    cursor.dwSize = 25;
    cursor.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(tui_output, &cursor);
    if (tui_win32_position(x, y, &pos))
        SetConsoleCursorPosition(tui_output, pos);
}

typedef struct TuiWin32DirtyContext {
    HANDLE output;
    CONSOLE_SCREEN_BUFFER_INFO info;
} TuiWin32DirtyContext;

static int tui_win32_write_run(int x, int y, int length,
                               const CHAR_INFO *cells,
                               void *context)
{
    COORD source;
    COORD buffer_size;
    SMALL_RECT target;
    TuiWin32DirtyContext *ctx;

    ctx = (TuiWin32DirtyContext *)context;
    source.X = 0;
    source.Y = 0;
    buffer_size.X = (SHORT)length;
    buffer_size.Y = 1;
    target.Left = (SHORT)(ctx->info.srWindow.Left + x);
    target.Top = (SHORT)(ctx->info.srWindow.Top + y);
    target.Right = (SHORT)(ctx->info.srWindow.Left + x + length - 1);
    target.Bottom = target.Top;
    return WriteConsoleOutputW(ctx->output, cells, buffer_size,
                               source, &target) ? 1 : 0;
}

void tui_console_present(void)
{
    TuiWin32DirtyContext context;

    if (tui_frame == 0 || tui_previous_frame == 0 ||
        !GetConsoleScreenBufferInfo(tui_output, &context.info))
        return;
    context.output = tui_output;
    if (tui_dirty_each_run(tui_frame, tui_previous_frame,
                           tui_frame_width, tui_frame_height,
                           tui_frame_dirty,
                           tui_win32_write_run, &context))
        tui_frame_dirty = 0;
}
