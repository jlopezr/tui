#include <windows.h>

#include "console.h"
#include "console_dirty.h"

#define TUI_MOUSE_DOUBLE_CLICK_MS 500
#define TUI_VT_ESC "\033["

static HANDLE tui_input;
static HANDLE tui_output;
static DWORD tui_input_mode;
static UINT tui_output_code_page;
static int tui_vt_ready;

static int tui_width;
static int tui_height;
static CHAR_INFO *tui_frame;
static CHAR_INFO *tui_previous_frame;
static char *tui_output_buffer;
static int tui_output_capacity;
static int tui_frame_dirty;
static int tui_cursor_x;
static int tui_cursor_y;
static int tui_cursor_visible;

static int tui_mouse_x;
static int tui_mouse_y;
static int tui_mouse_action;
static int tui_mouse_buttons;
static int tui_mouse_left_pressed;
static int tui_mouse_left_moved;
static int tui_mouse_press_x;
static int tui_mouse_press_y;
static DWORD tui_mouse_press_time;
static int tui_mouse_last_click_valid;
static int tui_mouse_last_click_x;
static int tui_mouse_last_click_y;
static DWORD tui_mouse_last_click_time;

static WCHAR tui_vt_glyph(int ch)
{
    switch (ch) {
    case TUI_CH_HLINE: return 0x2500;
    case TUI_CH_VLINE: return 0x2502;
    case TUI_CH_TL: return 0x250c;
    case TUI_CH_TR: return 0x2510;
    case TUI_CH_BL: return 0x2514;
    case TUI_CH_BR: return 0x2518;
    case TUI_CH_DHLINE: return 0x2550;
    case TUI_CH_DVLINE: return 0x2551;
    case TUI_CH_DTL: return 0x2554;
    case TUI_CH_DTR: return 0x2557;
    case TUI_CH_DBL: return 0x255a;
    case TUI_CH_DBR: return 0x255d;
    case TUI_CH_LTEE: return 0x251c;
    case TUI_CH_RTEE: return 0x2524;
    case TUI_CH_TTEE: return 0x252c;
    case TUI_CH_BTEE: return 0x2534;
    case TUI_CH_CROSS: return 0x253c;
    case TUI_CH_UP_TRIANGLE: return 0x25b2;
    case TUI_CH_DOWN_TRIANGLE: return 0x25bc;
    case TUI_CH_LEFT_TRIANGLE: return 0x25c0;
    case TUI_CH_RIGHT_TRIANGLE: return 0x25b6;
    case TUI_CH_SCROLL_TRACK: return 0x2592;
    case TUI_CH_SCROLL_THUMB: return 0x2588;
    case TUI_CH_CHECK: return 0x221a;
    case TUI_CH_BULLET: return 0x2022;
    default: return (WCHAR)(unsigned char)ch;
    }
}

static int tui_vt_append(char **cursor, char *end, const char *text)
{
    int length;

    length = lstrlenA(text);
    if (*cursor + length >= end)
        return 0;
    CopyMemory(*cursor, text, (SIZE_T)length);
    *cursor += length;
    return 1;
}

static int tui_vt_append_number(char **cursor, char *end, int value)
{
    char text[16];
    int length;
    int digit;

    length = 0;
    if (value == 0)
        text[length++] = '0';
    else {
        while (value > 0) {
            digit = value % 10;
            text[length++] = (char)('0' + digit);
            value /= 10;
        }
        for (digit = 0; digit < length / 2; ++digit) {
            char swap;

            swap = text[digit];
            text[digit] = text[length - digit - 1];
            text[length - digit - 1] = swap;
        }
    }
    text[length] = 0;
    return tui_vt_append(cursor, end, text);
}

static int tui_vt_append_utf8(char **cursor, char *end, WCHAR glyph)
{
    char text[8];
    int length;

    length = WideCharToMultiByte(CP_UTF8, 0, &glyph, 1,
                                 text, sizeof(text), 0, 0);
    if (length <= 0 || *cursor + length >= end)
        return 0;
    CopyMemory(*cursor, text, (SIZE_T)length);
    *cursor += length;
    return 1;
}

static int tui_vt_resize(int width, int height)
{
    CHAR_INFO *frame;
    CHAR_INFO *previous;
    char *output;
    int count;
    int i;

    if (width == tui_width && height == tui_height &&
        tui_frame != 0 && tui_output_buffer != 0)
        return 1;

    count = width * height;
    frame = (CHAR_INFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   (SIZE_T)count * sizeof(CHAR_INFO));
    previous = (CHAR_INFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      (SIZE_T)count * sizeof(CHAR_INFO));
    output = (char *)HeapAlloc(GetProcessHeap(), 0,
                               (SIZE_T)count * 20 + 64);
    if (frame == 0 || previous == 0 || output == 0) {
        if (frame != 0)
            HeapFree(GetProcessHeap(), 0, frame);
        if (previous != 0)
            HeapFree(GetProcessHeap(), 0, previous);
        if (output != 0)
            HeapFree(GetProcessHeap(), 0, output);
        return 0;
    }

    for (i = 0; i < count; ++i) {
        frame[i].Char.UnicodeChar = L' ';
        frame[i].Attributes = 0x07;
        previous[i].Char.UnicodeChar = 0;
        previous[i].Attributes = 0;
    }
    if (tui_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_frame);
    if (tui_previous_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_previous_frame);
    if (tui_output_buffer != 0)
        HeapFree(GetProcessHeap(), 0, tui_output_buffer);
    tui_frame = frame;
    tui_previous_frame = previous;
    tui_output_buffer = output;
    tui_output_capacity = count * 20 + 64;
    tui_width = width;
    tui_height = height;
    tui_frame_dirty = 1;
    return 1;
}

static int tui_vt_double_click(int x, int y)
{
    DWORD now;

    if (!tui_mouse_last_click_valid ||
        x != tui_mouse_last_click_x || y != tui_mouse_last_click_y)
        return 0;
    now = GetTickCount();
    tui_mouse_last_click_valid = 0;
    return now - tui_mouse_last_click_time <=
           TUI_MOUSE_DOUBLE_CLICK_MS;
}

static int tui_vt_mouse(const MOUSE_EVENT_RECORD *event)
{
    int left;
    int right;
    int middle;
    DWORD now;

    tui_mouse_x = event->dwMousePosition.X;
    tui_mouse_y = event->dwMousePosition.Y;
    tui_mouse_action = TUI_MOUSE_MOVE;
    tui_mouse_buttons = 0;
    left = (event->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
    right = (event->dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;
    middle = (event->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) != 0;

    if (event->dwEventFlags == DOUBLE_CLICK && left) {
        tui_mouse_action = TUI_MOUSE_DOUBLE;
        tui_mouse_buttons = TUI_MOUSE_LEFT;
    } else if (event->dwEventFlags == 0 && left &&
               !tui_mouse_left_pressed) {
        tui_mouse_action = tui_vt_double_click(
            tui_mouse_x, tui_mouse_y) ? TUI_MOUSE_DOUBLE : TUI_MOUSE_DOWN;
        tui_mouse_buttons = TUI_MOUSE_LEFT;
        tui_mouse_left_pressed = 1;
        tui_mouse_left_moved = 0;
        tui_mouse_press_x = tui_mouse_x;
        tui_mouse_press_y = tui_mouse_y;
        tui_mouse_press_time = GetTickCount();
    } else if (event->dwEventFlags == 0 && !left &&
               tui_mouse_left_pressed) {
        tui_mouse_action = TUI_MOUSE_UP;
        tui_mouse_buttons = TUI_MOUSE_LEFT;
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
    } else if (event->dwEventFlags == MOUSE_MOVED) {
        if (tui_mouse_left_pressed &&
            (tui_mouse_x != tui_mouse_press_x ||
             tui_mouse_y != tui_mouse_press_y))
            tui_mouse_left_moved = 1;
        if (left)
            tui_mouse_buttons = TUI_MOUSE_LEFT;
        else if (right)
            tui_mouse_buttons = TUI_MOUSE_RIGHT;
        else if (middle)
            tui_mouse_buttons = TUI_MOUSE_MIDDLE;
    } else if (right || middle) {
        tui_mouse_action = TUI_MOUSE_DOWN;
        tui_mouse_buttons = right ? TUI_MOUSE_RIGHT : TUI_MOUSE_MIDDLE;
    }
    return 1;
}

static int tui_vt_key(const KEY_EVENT_RECORD *event)
{
    switch (event->wVirtualKeyCode) {
    case VK_RETURN: return TUI_KEY_ENTER;
    case VK_ESCAPE: return TUI_KEY_ESCAPE;
    case VK_TAB:
        return (event->dwControlKeyState & SHIFT_PRESSED)
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
    case VK_F1: return TUI_KEY_F1; case VK_F2: return TUI_KEY_F2;
    case VK_F3: return TUI_KEY_F3; case VK_F4: return TUI_KEY_F4;
    case VK_F5: return TUI_KEY_F5; case VK_F6: return TUI_KEY_F6;
    case VK_F7: return TUI_KEY_F7; case VK_F8: return TUI_KEY_F8;
    case VK_F9: return TUI_KEY_F9; case VK_F10: return TUI_KEY_F10;
    case VK_F11: return TUI_KEY_F11; case VK_F12: return TUI_KEY_F12;
    default:
        return event->uChar.UnicodeChar != 0
            ? (int)event->uChar.UnicodeChar : TUI_KEY_NONE;
    }
}

int tui_console_init(void)
{
    DWORD output_mode;

    tui_input = GetStdHandle(STD_INPUT_HANDLE);
    tui_output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (tui_input == INVALID_HANDLE_VALUE ||
        tui_output == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(tui_input, &tui_input_mode) ||
        !GetConsoleMode(tui_output, &output_mode))
        return 0;
    if (!SetConsoleMode(tui_input, ENABLE_EXTENDED_FLAGS |
                        ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
        return 0;
    if (!SetConsoleMode(tui_output, output_mode |
                        ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                        ENABLE_PROCESSED_OUTPUT)) {
        SetConsoleMode(tui_input, tui_input_mode);
        return 0;
    }
    tui_output_code_page = GetConsoleOutputCP();
    if (!SetConsoleOutputCP(CP_UTF8)) {
        SetConsoleMode(tui_input, tui_input_mode);
        SetConsoleMode(tui_output, output_mode);
        return 0;
    }
    tui_width = 0;
    tui_height = 0;
    tui_frame = 0;
    tui_previous_frame = 0;
    tui_output_buffer = 0;
    tui_frame_dirty = 0;
    tui_cursor_x = 0;
    tui_cursor_y = 0;
    tui_cursor_visible = 0;
    tui_vt_ready = 1;
    WriteFile(tui_output, "\033[?1049h\033[2J\033[H\033[?25l",
              25, &output_mode, 0);
    return 1;
}

void tui_console_shutdown(void)
{
    DWORD written;

    if (!tui_vt_ready)
        return;
    WriteFile(tui_output, "\033[0m\033[2J\033[H\033[?25h\033[?1049l",
              31, &written, 0);
    SetConsoleMode(tui_input, tui_input_mode);
    SetConsoleOutputCP(tui_output_code_page);
    if (tui_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_frame);
    if (tui_previous_frame != 0)
        HeapFree(GetProcessHeap(), 0, tui_previous_frame);
    if (tui_output_buffer != 0)
        HeapFree(GetProcessHeap(), 0, tui_output_buffer);
    tui_frame = 0;
    tui_previous_frame = 0;
    tui_output_buffer = 0;
    tui_vt_ready = 0;
}

int tui_console_width(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(tui_output, &info))
        return 0;
    tui_width = info.srWindow.Right - info.srWindow.Left + 1;
    return tui_width;
}

int tui_console_height(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(tui_output, &info))
        return 0;
    tui_height = info.srWindow.Bottom - info.srWindow.Top + 1;
    return tui_height;
}

void tui_console_cell(int x, int y, int ch, int attr)
{
    int width;
    int height;

    width = tui_width;
    height = tui_height;
    if (!tui_vt_resize(width, height) ||
        x < 0 || x >= width || y < 0 || y >= height)
        return;
    tui_frame[y * width + x].Char.UnicodeChar = tui_vt_glyph(ch);
    tui_frame[y * width + x].Attributes =
        (WORD)((attr & 0x0f) | ((attr >> 4) & 0x0f) << 4);
    tui_frame_dirty = 1;
}

int tui_console_poll(void)
{
    INPUT_RECORD record;
    DWORD count;
    DWORD read;
    int key;

    for (;;) {
        if (!GetNumberOfConsoleInputEvents(tui_input, &count) || count == 0)
            return TUI_KEY_NONE;
        if (!ReadConsoleInputW(tui_input, &record, 1, &read) || read == 0)
            return TUI_KEY_NONE;

        key = TUI_KEY_NONE;
        if (record.EventType == KEY_EVENT &&
            record.Event.KeyEvent.bKeyDown)
            key = tui_vt_key(&record.Event.KeyEvent);
        else if (record.EventType == MOUSE_EVENT &&
                 tui_vt_mouse(&record.Event.MouseEvent))
            key = TUI_KEY_MOUSE;

        if (key != TUI_KEY_NONE)
            return key;
    }
}

int tui_console_key(void)
{
    int key;

    for (;;) {
        key = tui_console_poll();
        if (key != TUI_KEY_NONE)
            return key;
        if (WaitForSingleObject(tui_input, INFINITE) != WAIT_OBJECT_0)
            return TUI_KEY_NONE;
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
    tui_cursor_x = x;
    tui_cursor_y = y;
    tui_cursor_visible = visible;
}

typedef struct TuiVtDirtyContext {
    char **cursor;
    char *end;
    int previous_attr;
} TuiVtDirtyContext;

static int tui_vt_write_run(int x, int y, int length,
                            const CHAR_INFO *cells,
                            void *context)
{
    TuiVtDirtyContext *ctx;
    int i;
    int attr;
    int foreground;
    int background;

    ctx = (TuiVtDirtyContext *)context;
    if (!tui_vt_append(ctx->cursor, ctx->end, TUI_VT_ESC) ||
        !tui_vt_append_number(ctx->cursor, ctx->end, y + 1) ||
        !tui_vt_append(ctx->cursor, ctx->end, ";") ||
        !tui_vt_append_number(ctx->cursor, ctx->end, x + 1) ||
        !tui_vt_append(ctx->cursor, ctx->end, "H"))
        return 0;

    for (i = 0; i < length; ++i) {
        attr = cells[i].Attributes;
        if (attr != ctx->previous_attr) {
            foreground = (attr & 8) ? 90 : 30;
            foreground += attr & 0x07;
            background = (attr & 0x80) ? 100 : 40;
            background += (attr >> 4) & 0x07;
            if (!tui_vt_append(ctx->cursor, ctx->end, "\033[") ||
                !tui_vt_append_number(ctx->cursor, ctx->end, foreground) ||
                !tui_vt_append(ctx->cursor, ctx->end, ";") ||
                !tui_vt_append_number(ctx->cursor, ctx->end, background) ||
                !tui_vt_append(ctx->cursor, ctx->end, "m"))
                return 0;
            ctx->previous_attr = attr;
        }
        if (!tui_vt_append_utf8(ctx->cursor, ctx->end,
                                cells[i].Char.UnicodeChar))
            return 0;
    }
    return 1;
}

void tui_console_present(void)
{
    char *cursor;
    char *end;
    int previous_attr;
#ifndef TUI_PROFILE_FULL
    TuiVtDirtyContext dirty_context;
#endif
#ifdef TUI_PROFILE_FULL
    int x;
    int y;
    int attr;
    int foreground;
    int background;
    int start;
    int run_end;
#endif
    DWORD written;

    if (tui_frame == 0 || tui_previous_frame == 0 ||
        tui_output_buffer == 0)
        return;
    cursor = tui_output_buffer;
    end = cursor + tui_output_capacity;
    previous_attr = -1;
    if (tui_frame_dirty) {
#ifdef TUI_PROFILE_FULL
        for (y = 0; y < tui_height; ++y) {
            start = 0;
            run_end = tui_width;
            tui_vt_append(&cursor, end, TUI_VT_ESC);
            tui_vt_append_number(&cursor, end, y + 1);
            tui_vt_append(&cursor, end, ";1H");
            for (x = start; x < run_end; ++x) {
                attr = tui_frame[y * tui_width + x].Attributes;
                if (attr != previous_attr) {
                    foreground = (attr & 8) ? 90 : 30;
                    foreground += attr & 0x07;
                    background = (attr & 0x80) ? 100 : 40;
                    background += (attr >> 4) & 0x07;
                    tui_vt_append(&cursor, end, "\033[");
                    tui_vt_append_number(&cursor, end, foreground);
                    tui_vt_append(&cursor, end, ";");
                    tui_vt_append_number(&cursor, end, background);
                    tui_vt_append(&cursor, end, "m");
                    previous_attr = attr;
                }
                if (!tui_vt_append_utf8(
                        &cursor, end,
                        tui_frame[y * tui_width + x].Char.UnicodeChar))
                    return;
            }
        }
        CopyMemory(tui_previous_frame, tui_frame,
                   (SIZE_T)tui_width * (SIZE_T)tui_height *
                   sizeof(CHAR_INFO));
#else
        dirty_context.cursor = &cursor;
        dirty_context.end = end;
        dirty_context.previous_attr = previous_attr;
        if (!tui_dirty_each_run(tui_frame, tui_previous_frame,
                                tui_width, tui_height, tui_frame_dirty,
                                tui_vt_write_run, &dirty_context))
            return;
#endif
        tui_frame_dirty = 0;
    }
    tui_vt_append(&cursor, end, TUI_VT_ESC);
    tui_vt_append_number(&cursor, end, tui_cursor_y + 1);
    tui_vt_append(&cursor, end, ";");
    tui_vt_append_number(&cursor, end, tui_cursor_x + 1);
    tui_vt_append(&cursor, end, "H");
    tui_vt_append(&cursor, end,
                  tui_cursor_visible ? "\033[?25h" : "\033[?25l");
    WriteFile(tui_output, tui_output_buffer, (DWORD)(cursor -
              tui_output_buffer), &written, 0);
}
