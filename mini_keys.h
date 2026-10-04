#ifndef TUI_MINI_KEYS_H
#define TUI_MINI_KEYS_H

/*
 * Teclas fisicas de INPUT (mmio.md 25) -> teclas del TUI, para el teclado
 * ESPANOL.
 *
 * Es codigo puro, sin MMIO, y vive en un .h con funciones `static` por dos
 * razones: lo incluye console_mini.c (que se compila como una sola unidad con
 * mini-lcc) y lo incluyen tambien las pruebas del PC, que asi comprueban la
 * misma tabla que se ejecuta en la placa.
 *
 * INPUT entrega Usage IDs FISICOS, no caracteres (25.12): la tecla a la derecha
 * de la L es el Usage 0x33 tanto en un teclado americano (`;`) como en uno
 * espanol (`n` con tilde). Quien da el significado es esta tabla.
 *
 * Los caracteres salen en CP437, que es lo que dibuja la fuente de la consola:
 * la enie es 0xA4, la `?` invertida 0xA8, la `c` con cedilla 0x87.
 *
 * Lo que NO hace:
 *   - Mayusculas acentuadas que CP437 no tiene (A, I, O, U con acento agudo):
 *     salen sin acento. Si tiene (E con agudo, enie, A/O/U con dieresis).
 *   - Un acento muerto seguido de una letra que no admite acento: se pierde el
 *     acento y sale la letra. (Windows sacaria las dos.) Con la barra espaciadora
 *     sale el acento solo.
 *   - AltGr+E (el euro) y demas simbolos fuera de CP437.
 *   - Bloq Num: el teclado numerico se toma como si estuviera activado.
 * Bloq Mayus SI: lo lleva quien llama, y llega en el bit MINI_MOD_CAPS.
 */

#include "console.h"

/* Bits de MODIFIERS en un evento KEY (mmio.md 25.6): el bit n es el Usage 0xE0+n. */
#define MINI_MOD_LCTRL   0x01
#define MINI_MOD_LSHIFT  0x02
#define MINI_MOD_LALT    0x04
#define MINI_MOD_RCTRL   0x10
#define MINI_MOD_RSHIFT  0x20
#define MINI_MOD_ALTGR   0x40         /* Alt derecho */
/* No es de INPUT: lo anade quien llama cuando Bloq Mayus esta activo. */
#define MINI_MOD_CAPS    0x100

#define MINI_MOD_SHIFT   (MINI_MOD_LSHIFT | MINI_MOD_RSHIFT)
#define MINI_MOD_CTRL    (MINI_MOD_LCTRL | MINI_MOD_RCTRL)

/* Usage ID de Bloq Mayus: quien llama alterna su estado al pulsarla. */
#define MINI_USAGE_CAPSLOCK  0x39

/* Acentos muertos: valores internos, nunca salen de mini_key_translate. */
#define MINI_DEAD_NONE    0
#define MINI_DEAD_GRAVE   0x201
#define MINI_DEAD_ACUTE   0x202
#define MINI_DEAD_CIRC    0x203
#define MINI_DEAD_DIAER   0x204

static int mini_is_letter(int usage)
{
    return usage >= 0x04 && usage <= 0x1d;
}

/* Digitos: sin modificador, con Mays y con AltGr. */
static int mini_digit_key(int usage, int mods)
{
    static const unsigned char plain[10] = {
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
    };
    /* ! " (punto medio) $ % & / ( ) = */
    static const unsigned char shifted[10] = {
        '!', '"', 0xfa, '$', '%', '&', '/', '(', ')', '='
    };
    int index = usage - 0x1e;

    if (mods & MINI_MOD_ALTGR) {
        switch (index) {
        case 0: return '|';
        case 1: return '@';
        case 2: return '#';
        case 3: return '~';
        case 5: return 0xaa;          /* negacion */
        default: return 0;
        }
    }
    if (mods & MINI_MOD_SHIFT)
        return shifted[index];
    return plain[index];
}

/*
 * Las teclas de simbolos del teclado espanol (ISO). Devuelve un caracter
 * CP437, un acento muerto (MINI_DEAD_*) o 0 si la combinacion no existe.
 */
static int mini_symbol_key(int usage, int mods)
{
    int shift = (mods & MINI_MOD_SHIFT) != 0;
    int altgr = (mods & MINI_MOD_ALTGR) != 0;

    switch (usage) {
    case 0x2d: return altgr ? '\\' : (shift ? '?' : '\'');
    case 0x2e: return shift ? 0xa8 : 0xad;                   /* ? ! invertidas */
    case 0x2f: return altgr ? '[' : (shift ? MINI_DEAD_CIRC : MINI_DEAD_GRAVE);
    case 0x30: return altgr ? ']' : (shift ? '*' : '+');
    case 0x31:
    case 0x32: return altgr ? '}' : (shift ? 0x80 : 0x87);   /* C cedilla */
    case 0x33: return shift ? 0xa5 : 0xa4;                   /* enie */
    case 0x34: return altgr ? '{' : (shift ? MINI_DEAD_DIAER : MINI_DEAD_ACUTE);
    case 0x35: return altgr ? '\\' : (shift ? 0xa6 : 0xa7);  /* ordinales */
    case 0x36: return shift ? ';' : ',';
    case 0x37: return shift ? ':' : '.';
    case 0x38: return shift ? '_' : '-';
    case 0x64: return shift ? '>' : '<';
    default: return 0;
    }
}

/* Letra con acento muerto, o la letra tal cual si no admite acento en CP437. */
static int mini_compose(int dead, int letter)
{
    switch (dead) {
    case MINI_DEAD_ACUTE:
        switch (letter) {
        case 'a': return 0xa0;
        case 'e': return 0x82;
        case 'i': return 0xa1;
        case 'o': return 0xa2;
        case 'u': return 0xa3;
        case 'E': return 0x90;
        default: return letter;
        }
    case MINI_DEAD_GRAVE:
        switch (letter) {
        case 'a': return 0x85;
        case 'e': return 0x8a;
        case 'i': return 0x8d;
        case 'o': return 0x95;
        case 'u': return 0x97;
        default: return letter;
        }
    case MINI_DEAD_CIRC:
        switch (letter) {
        case 'a': return 0x83;
        case 'e': return 0x88;
        case 'i': return 0x8c;
        case 'o': return 0x93;
        case 'u': return 0x96;
        default: return letter;
        }
    case MINI_DEAD_DIAER:
        switch (letter) {
        case 'a': return 0x84;
        case 'e': return 0x89;
        case 'i': return 0x8b;
        case 'o': return 0x94;
        case 'u': return 0x81;
        case 'y': return 0x98;
        case 'A': return 0x8e;
        case 'O': return 0x99;
        case 'U': return 0x9a;
        default: return letter;
        }
    default:
        return letter;
    }
}

/* El acento solo, para cuando se pulsa la barra espaciadora tras un muerto. */
static int mini_dead_char(int dead)
{
    switch (dead) {
    case MINI_DEAD_ACUTE: return '\'';
    case MINI_DEAD_GRAVE: return '`';
    case MINI_DEAD_CIRC: return '^';
    case MINI_DEAD_DIAER: return '"';
    default: return 0;
    }
}

/* Teclas que no son caracteres: las mismas con o sin modificadores. */
static int mini_function_key(int usage, int mods)
{
    if (usage >= 0x3a && usage <= 0x45)
        return TUI_KEY_F1 + (usage - 0x3a);

    switch (usage) {
    case 0x28:
    case 0x58: return TUI_KEY_ENTER;
    case 0x29: return TUI_KEY_ESCAPE;
    case 0x2a: return TUI_KEY_BACKSPACE;
    case 0x2b: return (mods & MINI_MOD_SHIFT) ? TUI_KEY_BACKTAB : TUI_KEY_TAB;
    case 0x49: return TUI_KEY_INSERT;
    case 0x4a: return TUI_KEY_HOME;
    case 0x4b: return TUI_KEY_PAGEUP;
    case 0x4c: return TUI_KEY_DELETE;
    case 0x4d: return TUI_KEY_END;
    case 0x4e: return TUI_KEY_PAGEDOWN;
    case 0x4f: return TUI_KEY_RIGHT;
    case 0x50: return TUI_KEY_LEFT;
    case 0x51: return TUI_KEY_DOWN;
    case 0x52: return TUI_KEY_UP;
    default: return TUI_KEY_NONE;
    }
}

/* Teclado numerico (Bloq Num activo): 0x54 / 0x55 * 0x56 - 0x57 + 0x59.. 1-9 0x62 0 0x63 . */
static int mini_keypad_key(int usage)
{
    switch (usage) {
    case 0x54: return '/';
    case 0x55: return '*';
    case 0x56: return '-';
    case 0x57: return '+';
    case 0x62: return '0';
    case 0x63: return '.';
    default:
        if (usage >= 0x59 && usage <= 0x61)
            return '1' + (usage - 0x59);
        return 0;
    }
}

/*
 * La pulsacion de una tecla: Usage ID y MODIFIERS del evento (mas
 * MINI_MOD_CAPS si Bloq Mayus esta activo). `*dead` guarda el acento muerto
 * pendiente entre llamadas; empieza en MINI_DEAD_NONE.
 *
 * Devuelve un caracter CP437 (1..255), un TUI_KEY_*, o TUI_KEY_NONE si la
 * pulsacion no produce nada (un acento muerto, una tecla desconocida).
 */
static int mini_key_translate(int usage, int mods, int *dead)
{
    int pending = *dead;
    int key;

    *dead = MINI_DEAD_NONE;

    key = mini_function_key(usage, mods);
    if (key != TUI_KEY_NONE)
        return key;

    if (usage == 0x2c)
        return pending != MINI_DEAD_NONE ? mini_dead_char(pending) : ' ';

    if (mini_is_letter(usage)) {
        int upper = ((mods & MINI_MOD_SHIFT) != 0) != ((mods & MINI_MOD_CAPS) != 0);

        key = 'a' + (usage - 0x04);
        if (mods & MINI_MOD_CTRL)
            return key - 'a' + 1;     /* Ctrl+A = 1 ... Ctrl+Z = 26 */
        if (upper)
            key = key - 'a' + 'A';
        return mini_compose(pending, key);
    }

    key = mini_keypad_key(usage);
    if (key == 0) {
        if (usage >= 0x1e && usage <= 0x27)
            key = mini_digit_key(usage, mods);
        else
            key = mini_symbol_key(usage, mods);
    }

    if (key >= MINI_DEAD_GRAVE) {
        *dead = key;                  /* se espera la letra siguiente */
        return TUI_KEY_NONE;
    }
    return key;
}

#endif
