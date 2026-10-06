#include "console.h"
#include "mini_keys.h"

#define MINI_UART_DATA       (*(volatile unsigned int *)0x80100000)
#define MINI_UART_STATUS     (*(volatile unsigned int *)0x80100004)
#define MINI_VIDEO_COMMIT    (*(volatile unsigned int *)0x8020000c)
#define MINI_VIDEO_CONFIG    (*(volatile unsigned int *)0x80200040)
#define MINI_VIDEO_FRAMES    (*(volatile unsigned int *)0x80200014)
#define MINI_VIDEO_PALETTE   ((volatile unsigned int *)0x80201000)
#define MINI_TEXT_RAM        ((volatile unsigned int *)0x80206000)

/* SYSTEM.DEVICES (mmio.md 5.4): el bit 11 dice que hay bloque INPUT. */
#define MINI_SYS_DEVICES     (*(volatile unsigned int *)0x8000000c)
#define MINI_DEV_INPUT       0x800

/* INPUT (mmio.md 25), en 0x80600000. */
#define MINI_IN_EVENT        (*(volatile unsigned int *)0x80600000)
#define MINI_IN_STATUS       (*(volatile unsigned int *)0x80600004)
#define MINI_IN_KEY_STATE    ((volatile unsigned int *)0x80600010)
#define MINI_IN_MOUSE_PRESENT 0x40000   /* STATUS bit 18; el bit 17 es el teclado */

#define MINI_WIDTH           80
#define MINI_HEIGHT          30
#define MINI_CELLS           (MINI_WIDTH * MINI_HEIGHT)
#define MINI_TEXT_ENABLE     4
#define MINI_STATE_COMMIT    2

#define MINI_FG_MASK         0x0f00
#define MINI_BG_MASK         0xf000

/* Tipos de evento de INPUT (mmio.md 25.5) y su campo DOWN. */
#define MINI_EV_KEY          0
#define MINI_EV_BUTTON       1
#define MINI_EV_MOVE         2

/*
 * El ratón de INPUT es relativo y la consola mide 640x480 puntos de pantalla:
 * un punto de movimiento es un pixel, y una celda son 8x16.
 */
#define MINI_SCREEN_W        (MINI_WIDTH * 8)
#define MINI_SCREEN_H        (MINI_HEIGHT * 16)

/*
 * Tiempos, en frames de video (60 por segundo): FRAME_COUNT cuenta aunque la CPU
 * este ocupada y existe tambien en el simulador. El contrato no define
 * typematic ni doble clic (mmio.md 25.10), asi que son de esta consola.
 */
#define MINI_REPEAT_DELAY    24     /* 400 ms hasta la primera repeticion */
#define MINI_REPEAT_PERIOD   2      /* ~30 repeticiones por segundo */
#define MINI_DOUBLE_CLICK    30     /* 500 ms, como el backend VT de Windows */

/*
 * Lo ultimo que se escribio en cada celda, como (caracter | atributo << 9).
 * La CPU hace ~4,5 MIPS efectivos y cada evento de entrada redibuja las 2400
 * celdas: casi todas con lo mismo que ya tenian. Saltarse esas escrituras --
 * que son accesos MMIO-- es lo que hace usable el teclado y el raton. La copia
 * es del contenido LOGICO: el puntero y el cursor se dibujan encima y no la
 * tocan, asi que una celda igual bajo ellos tampoco se reescribe.
 */
static int mini_shadow[MINI_CELLS];

static int mini_cursor_x;
static int mini_cursor_y;
static int mini_cursor_visible;
static int mini_cursor_drawn;
static unsigned int mini_cursor_under;

/* Entrada por INPUT. Sin el bloque (bitstream o simulador sin teclado), solo serie. */
static int mini_input_enabled;
static int mini_dead_key;           /* acento muerto pendiente (mini_keys.h) */
static int mini_caps;               /* Bloq Mayus activo */

/* Repeticion de la ultima tecla, mientras siga pulsada. */
static int mini_repeat_usage;
static int mini_repeat_mods;
static int mini_repeat_started;
static unsigned int mini_repeat_last;

/* Ratón: posicion en pixeles, y el evento pendiente para tui_console_mouse. */
static int mini_pointer_px;
static int mini_pointer_py;
static int mini_pointer_visible;
static int mini_pointer_drawn;
static int mini_pointer_cell;
static unsigned int mini_pointer_under;
static int mini_mouse_x;
static int mini_mouse_y;
static int mini_mouse_action;
static int mini_mouse_buttons;

/* Ultimo clic izquierdo, para el doble clic. */
static int mini_click_valid;
static int mini_click_x;
static int mini_click_y;
static unsigned int mini_click_frame;

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

/*
 * El caracter se enmascara con `& 0xff` y NO con `(unsigned char)`: mini-lcc
 * compila ese cast sobre un `int` como un no-op, asi que un char con signo
 * (la enie, 0xA4, leida de un buffer) llegaba como 0xFFFFFFA4 y la RAM de texto
 * rechazaba la escritura con un error de MMIO. Con solo ASCII no se notaba.
 */
static unsigned int mini_cell(int ch, int attr)
{
    return ((unsigned int)mini_slot[attr & 0x0f] << 8) |
           ((unsigned int)mini_slot[(attr >> 4) & 0x0f] << 12) |
           (unsigned int)(ch & 0xff);
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

/*
 * Los dos adornos que se dibujan SOBRE el texto --el cursor de edicion y el
 * puntero del raton-- guardan lo que tapan y lo restauran antes de que nadie
 * escriba una celda. Se muestran y se ocultan en orden inverso (cursor,
 * puntero / puntero, cursor), porque pueden caer en la misma celda.
 */
static void mini_pointer_hide(void)
{
    if (mini_pointer_drawn) {
        MINI_TEXT_RAM[mini_pointer_cell] = mini_pointer_under;
        mini_pointer_drawn = 0;
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

static void mini_overlay_hide(void)
{
    mini_pointer_hide();
    mini_cursor_hide();
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

/* El puntero invierte colores de la celda: la letra se sigue leyendo. */
static void mini_pointer_show(void)
{
    unsigned int cell;

    if (!mini_pointer_visible)
        return;

    mini_pointer_cell = (mini_pointer_py >> 4) * MINI_WIDTH +
                        (mini_pointer_px >> 3);
    cell = MINI_TEXT_RAM[mini_pointer_cell];
    mini_pointer_under = cell;
    MINI_TEXT_RAM[mini_pointer_cell] =
        (cell & 0x00ff) |
        ((cell & MINI_FG_MASK) << 4) |
        ((cell & MINI_BG_MASK) >> 4);
    mini_pointer_drawn = 1;
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

/* La tecla que llega por la UART, ya traducida. */
static int mini_serial_key(void)
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

/* ------------------------------------------------------------------------- */
/* INPUT: teclado                                                            */
/* ------------------------------------------------------------------------- */

static int mini_key_is_down(int usage)
{
    return (int)((MINI_IN_KEY_STATE[usage >> 5] >> (usage & 31)) & 1);
}

/*
 * Una pulsacion de INPUT. Los modificadores (KEY = 0) y las liberaciones no
 * producen tecla. Bloq Mayus se alterna aqui: llega como una tecla mas.
 */
static int mini_input_key_event(unsigned int event)
{
    int usage;
    int down;
    int mods;
    int key;

    usage = (int)(event & 0xff);
    down = (int)((event >> 8) & 1);
    mods = (int)((event >> 16) & 0xff);

    if (usage == 0)
        return TUI_KEY_NONE;

    if (!down) {
        if (usage == mini_repeat_usage)
            mini_repeat_usage = 0;
        return TUI_KEY_NONE;
    }

    if (usage == MINI_USAGE_CAPSLOCK) {
        mini_caps = !mini_caps;
        return TUI_KEY_NONE;
    }

    if (mini_caps)
        mods |= MINI_MOD_CAPS;
    key = mini_key_translate(usage, mods, &mini_dead_key);
    if (key != TUI_KEY_NONE) {
        mini_repeat_usage = usage;
        mini_repeat_mods = mods;
        mini_repeat_started = 0;
        mini_repeat_last = MINI_VIDEO_FRAMES;
    } else {
        mini_repeat_usage = 0;
    }
    return key;
}

/* Si la ultima tecla sigue pulsada, la repite pasado su tiempo. */
static int mini_input_repeat(void)
{
    unsigned int now;
    int wait;
    int dead;

    if (mini_repeat_usage == 0)
        return TUI_KEY_NONE;
    if (!mini_key_is_down(mini_repeat_usage)) {
        mini_repeat_usage = 0;
        return TUI_KEY_NONE;
    }

    now = MINI_VIDEO_FRAMES;
    wait = mini_repeat_started ? MINI_REPEAT_PERIOD : MINI_REPEAT_DELAY;
    if ((int)(now - mini_repeat_last) < wait)
        return TUI_KEY_NONE;

    mini_repeat_last = now;
    mini_repeat_started = 1;
    dead = MINI_DEAD_NONE;          /* un acento muerto no se repite */
    return mini_key_translate(mini_repeat_usage, mini_repeat_mods, &dead);
}

/* ------------------------------------------------------------------------- */
/* INPUT: raton                                                              */
/* ------------------------------------------------------------------------- */

static int mini_signed12(unsigned int value)
{
    int v;

    v = (int)(value & 0xfff);
    if (v & 0x800)
        v -= 0x1000;
    return v;
}

static int mini_clamp(int value, int limit)
{
    if (value < 0)
        return 0;
    if (value >= limit)
        return limit - 1;
    return value;
}

static int mini_mouse_event_ready(int x, int y, int action, int buttons)
{
    mini_mouse_x = x;
    mini_mouse_y = y;
    mini_mouse_action = action;
    mini_mouse_buttons = buttons;
    return TUI_KEY_MOUSE;
}

/*
 * Movimiento: se acumulan los puntos y solo hay evento para el TUI si cambia la
 * CELDA, que es la unica resolucion que el ve. Sin esto, cada evento del raton
 * (decenas por segundo) provocaria un redibujado entero. Los movimientos que
 * llegan seguidos los junta tui_read_event().
 */
static int mini_input_move_event(unsigned int event)
{
    int old_x;
    int old_y;

    old_x = mini_pointer_px >> 3;
    old_y = mini_pointer_py >> 4;
    mini_pointer_px = mini_clamp(mini_pointer_px + mini_signed12(event),
                                 MINI_SCREEN_W);
    mini_pointer_py = mini_clamp(mini_pointer_py + mini_signed12(event >> 12),
                                 MINI_SCREEN_H);
    mini_pointer_visible = 1;

    if ((mini_pointer_px >> 3) == old_x && (mini_pointer_py >> 4) == old_y)
        return TUI_KEY_NONE;

    /*
     * El puntero se mueve YA, sin esperar a que el TUI redibuje la pantalla
     * (cientos de milisegundos): el raton tiene que sentirse inmediato aunque
     * lo que hay debajo tarde en actualizarse.
     */
    mini_pointer_hide();
    mini_pointer_show();
    return mini_mouse_event_ready(mini_pointer_px >> 3, mini_pointer_py >> 4,
                                  TUI_MOUSE_MOVE, 0);
}

/* Doble clic: el segundo clic izquierdo en la misma celda dentro del plazo. */
static int mini_is_double_click(int x, int y, unsigned int now)
{
    int valid;

    valid = mini_click_valid && x == mini_click_x && y == mini_click_y &&
            (int)(now - mini_click_frame) <= MINI_DOUBLE_CLICK;
    mini_click_valid = 0;
    return valid;
}

static int mini_input_button_event(unsigned int event)
{
    int button;
    int down;
    int x;
    int y;
    unsigned int now;

    button = (int)(event & 0xff);
    down = (int)((event >> 8) & 1);
    if (button > 2)
        return TUI_KEY_NONE;        /* INPUT tiene hasta 32; el TUI usa tres */

    mini_pointer_visible = 1;
    x = mini_pointer_px >> 3;
    y = mini_pointer_py >> 4;

    if (!down)
        return mini_mouse_event_ready(x, y, TUI_MOUSE_UP, 1 << button);

    now = MINI_VIDEO_FRAMES;
    if (button == 0) {
        if (mini_is_double_click(x, y, now))
            return mini_mouse_event_ready(x, y, TUI_MOUSE_DOUBLE, 1);
        mini_click_valid = 1;
        mini_click_x = x;
        mini_click_y = y;
        mini_click_frame = now;
    }
    return mini_mouse_event_ready(x, y, TUI_MOUSE_DOWN, 1 << button);
}

/* El siguiente evento de INPUT que le interese al TUI, o TUI_KEY_NONE. */
static int mini_input_poll(void)
{
    unsigned int event;
    int key;

    while ((MINI_IN_STATUS & 0xffff) != 0) {
        event = MINI_IN_EVENT;
        key = TUI_KEY_NONE;
        switch ((int)((event >> 24) & 0xff)) {
        case MINI_EV_KEY: key = mini_input_key_event(event); break;
        case MINI_EV_BUTTON: key = mini_input_button_event(event); break;
        case MINI_EV_MOVE: key = mini_input_move_event(event); break;
        default: break;             /* tipos reservados: se descartan */
        }
        if (key != TUI_KEY_NONE)
            return key;
    }
    return mini_input_repeat();
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
    for (i = 0; i < MINI_CELLS; ++i) {
        MINI_TEXT_RAM[i] = mini_cell(' ', 0x07);
        mini_shadow[i] = ' ' | (0x07 << 9);
    }

    mini_cursor_x = 0;
    mini_cursor_y = 0;
    mini_cursor_visible = 0;
    mini_cursor_drawn = 0;

    /*
     * Leer INPUT en un sistema que no lo tiene es un error de MMIO y la CPU se
     * para, asi que se mira antes en SYSTEM.DEVICES.
     */
    mini_input_enabled = (MINI_SYS_DEVICES & MINI_DEV_INPUT) != 0;
    mini_dead_key = MINI_DEAD_NONE;
    mini_caps = 0;
    mini_repeat_usage = 0;
    mini_pointer_px = MINI_SCREEN_W / 2;     /* donde la ventana del PC supone el raton */
    mini_pointer_py = MINI_SCREEN_H / 2;
    mini_pointer_visible = 0;
    mini_pointer_drawn = 0;
    mini_click_valid = 0;

    mini_commit_video(1);
    return 1;
}

void tui_console_shutdown(void)
{
    mini_overlay_hide();
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
    int index;
    int packed;

    if (x < 0 || x >= MINI_WIDTH || y < 0 || y >= MINI_HEIGHT)
        return;
    index = y * MINI_WIDTH + x;
    packed = (ch & 0x1ff) | ((attr & 0xff) << 9);
    if (mini_shadow[index] == packed)
        return;
    mini_shadow[index] = packed;
    mini_overlay_hide();
    MINI_TEXT_RAM[index] = mini_cell(mini_glyph(ch), attr);
}

/*
 * La siguiente tecla de la UART o de INPUT, la que este antes, o TUI_KEY_NONE si
 * no hay ninguna. tui_console_key() espera sondeando las dos: nunca bloquea en una.
 */
int tui_console_poll(void)
{
    if (mini_serial_available())
        return mini_serial_key();
    if (mini_input_enabled)
        return mini_input_poll();
    return TUI_KEY_NONE;
}

int tui_console_key(void)
{
    int key;

    for (;;) {
        key = tui_console_poll();
        if (key != TUI_KEY_NONE)
            return key;
    }
}

void tui_console_mouse(int *x, int *y, int *action, int *buttons)
{
    *x = mini_mouse_x;
    *y = mini_mouse_y;
    *action = mini_mouse_action ? mini_mouse_action : TUI_MOUSE_MOVE;
    *buttons = mini_mouse_buttons;
}

/*
 * El bloque INPUT existe o no desde que arranca el sistema (SYSTEM.DEVICES, que
 * se mira en tui_console_init), pero el RATON lo conecta o desconecta quien lo
 * alimenta --`monitor.py input`-- con el programa en marcha: la presencia se lee
 * del STATUS cada vez, que es una sola lectura de MMIO.
 */
int tui_console_has_mouse(void)
{
    return mini_input_enabled && (MINI_IN_STATUS & MINI_IN_MOUSE_PRESENT) != 0;
}

/* La consola dibuja CP437: ademas del ASCII, las letras y signos de 128..255. */
int tui_console_printable(int key)
{
    return TUI_ASCII_PRINTABLE(key) || (key >= 128 && key <= 255);
}

#ifdef TUI_PROFILE_EVENTS
/* Para la medicion de demo.c: eventos esperando ahora (UART + INPUT) y el reloj. */
int tui_console_pending(void)
{
    int count;

    count = (int)(MINI_UART_STATUS & 0xff);
    if (mini_input_enabled)
        count += (int)(MINI_IN_STATUS & 0xffff);
    return count;
}

unsigned int tui_console_frames(void)
{
    return MINI_VIDEO_FRAMES;
}
#endif

void tui_console_cursor(int x, int y, int visible)
{
    mini_overlay_hide();
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
    mini_overlay_hide();
    mini_cursor_show();
    mini_pointer_show();
}
