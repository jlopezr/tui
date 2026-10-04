#include <string.h>

#include "test_support.h"
#include "../mini_keys.h"

/* Usage IDs fisicos que usan las pruebas. */
#define U_A       0x04
#define U_E       0x08
#define U_U       0x18
#define U_1       0x1e
#define U_2       0x1f
#define U_4       0x21
#define U_7       0x24
#define U_0       0x27
#define U_ENTER   0x28
#define U_TAB     0x2b
#define U_SPACE   0x2c
#define U_APOS    0x2d      /* ' ? */
#define U_INVERT  0x2e      /* inverted ! ? */
#define U_GRAVE   0x2f      /* ` ^ */
#define U_PLUS    0x30      /* + * */
#define U_CCEDIL  0x31
#define U_ENIE    0x33
#define U_ACUTE   0x34      /* acute diaeresis */
#define U_ORD     0x35
#define U_COMMA   0x36
#define U_DOT     0x37
#define U_MINUS   0x38
#define U_LESS    0x64

static int press(int usage, int mods)
{
    static int dead;

    return mini_key_translate(usage, mods, &dead);
}

static void test_letters(void)
{
    CHECK(press(U_A, 0) == 'a');
    CHECK(press(U_A, MINI_MOD_LSHIFT) == 'A');
    CHECK(press(U_A, MINI_MOD_RSHIFT) == 'A');
    CHECK(press(0x1d, 0) == 'z');
    CHECK(press(U_A, MINI_MOD_CAPS) == 'A');
    /* Mayusculas y Bloq Mayus se cancelan, como en cualquier teclado. */
    CHECK(press(U_A, MINI_MOD_CAPS | MINI_MOD_LSHIFT) == 'a');
    CHECK(press(U_A, MINI_MOD_LCTRL) == 1);
    CHECK(press(0x1d, MINI_MOD_RCTRL) == 26);
}

static void test_digits_and_shifted_digits(void)
{
    CHECK(press(U_1, 0) == '1');
    CHECK(press(U_0, 0) == '0');
    CHECK(press(U_1, MINI_MOD_LSHIFT) == '!');
    CHECK(press(U_2, MINI_MOD_LSHIFT) == '"');
    CHECK(press(U_4, MINI_MOD_LSHIFT) == '$');
    CHECK(press(U_7, MINI_MOD_LSHIFT) == '/');
    CHECK(press(U_0, MINI_MOD_LSHIFT) == '=');
    /* Bloq Mayus no toca los digitos. */
    CHECK(press(U_1, MINI_MOD_CAPS) == '1');
}

static void test_altgr_symbols(void)
{
    CHECK(press(U_1, MINI_MOD_ALTGR) == '|');
    CHECK(press(U_2, MINI_MOD_ALTGR) == '@');
    CHECK(press(0x20, MINI_MOD_ALTGR) == '#');
    CHECK(press(U_4, MINI_MOD_ALTGR) == '~');
    CHECK(press(U_APOS, MINI_MOD_ALTGR) == '\\');
    CHECK(press(U_GRAVE, MINI_MOD_ALTGR) == '[');
    CHECK(press(U_PLUS, MINI_MOD_ALTGR) == ']');
    CHECK(press(U_ACUTE, MINI_MOD_ALTGR) == '{');
    CHECK(press(U_CCEDIL, MINI_MOD_ALTGR) == '}');
    CHECK(press(U_ORD, MINI_MOD_ALTGR) == '\\');
    /* AltGr+5 es el euro, que CP437 no tiene: no sale nada. */
    CHECK(press(U_1 + 4, MINI_MOD_ALTGR) == 0);
}

static void test_spanish_symbols(void)
{
    CHECK(press(U_APOS, 0) == '\'');
    CHECK(press(U_APOS, MINI_MOD_LSHIFT) == '?');
    CHECK(press(U_INVERT, 0) == 0xad);
    CHECK(press(U_INVERT, MINI_MOD_LSHIFT) == 0xa8);
    CHECK(press(U_PLUS, 0) == '+');
    CHECK(press(U_PLUS, MINI_MOD_LSHIFT) == '*');
    CHECK(press(U_COMMA, 0) == ',');
    CHECK(press(U_COMMA, MINI_MOD_LSHIFT) == ';');
    CHECK(press(U_DOT, 0) == '.');
    CHECK(press(U_DOT, MINI_MOD_LSHIFT) == ':');
    CHECK(press(U_MINUS, 0) == '-');
    CHECK(press(U_MINUS, MINI_MOD_LSHIFT) == '_');
    CHECK(press(U_LESS, 0) == '<');
    CHECK(press(U_LESS, MINI_MOD_LSHIFT) == '>');
    CHECK(press(U_ORD, 0) == 0xa7);
    CHECK(press(U_ORD, MINI_MOD_LSHIFT) == 0xa6);
}

static void test_enie_and_cedilla(void)
{
    CHECK(press(U_ENIE, 0) == 0xa4);
    CHECK(press(U_ENIE, MINI_MOD_LSHIFT) == 0xa5);
    CHECK(press(U_CCEDIL, 0) == 0x87);
    CHECK(press(U_CCEDIL, MINI_MOD_LSHIFT) == 0x80);
    /* La tecla extra de algunos teclados ISO (Non-US #) hace lo mismo. */
    CHECK(press(0x32, 0) == 0x87);
    /* Y las dos caen donde los controles de texto aceptan caracteres. */
    CHECK(press(U_ENIE, 0) > 127 && press(U_ENIE, 0) < 256);
}

static void test_dead_keys_compose(void)
{
    int dead = MINI_DEAD_NONE;

    /* El acento solo no escribe nada... */
    CHECK(mini_key_translate(U_ACUTE, 0, &dead) == TUI_KEY_NONE);
    CHECK(dead == MINI_DEAD_ACUTE);
    /* ...y la vocal siguiente sale acentuada. */
    CHECK(mini_key_translate(U_A, 0, &dead) == 0xa0);
    CHECK(dead == MINI_DEAD_NONE);

    mini_key_translate(U_ACUTE, 0, &dead);
    CHECK(mini_key_translate(U_E, 0, &dead) == 0x82);
    mini_key_translate(U_ACUTE, 0, &dead);
    CHECK(mini_key_translate(U_E, MINI_MOD_LSHIFT, &dead) == 0x90);
    mini_key_translate(U_ACUTE, 0, &dead);
    CHECK(mini_key_translate(U_U, 0, &dead) == 0xa3);

    /* Dieresis: Mays + la tecla del acento. */
    mini_key_translate(U_ACUTE, MINI_MOD_LSHIFT, &dead);
    CHECK(dead == MINI_DEAD_DIAER);
    CHECK(mini_key_translate(U_U, 0, &dead) == 0x81);
    mini_key_translate(U_ACUTE, MINI_MOD_LSHIFT, &dead);
    CHECK(mini_key_translate(U_U, MINI_MOD_LSHIFT, &dead) == 0x9a);

    /* Grave y circunflejo. */
    mini_key_translate(U_GRAVE, 0, &dead);
    CHECK(mini_key_translate(U_A, 0, &dead) == 0x85);
    mini_key_translate(U_GRAVE, MINI_MOD_LSHIFT, &dead);
    CHECK(mini_key_translate(U_A, 0, &dead) == 0x83);
}

static void test_dead_keys_edge_cases(void)
{
    int dead = MINI_DEAD_NONE;

    /* Con la barra espaciadora sale el acento solo. */
    mini_key_translate(U_ACUTE, 0, &dead);
    CHECK(mini_key_translate(U_SPACE, 0, &dead) == '\'');
    mini_key_translate(U_GRAVE, 0, &dead);
    CHECK(mini_key_translate(U_SPACE, 0, &dead) == '`');
    mini_key_translate(U_GRAVE, MINI_MOD_LSHIFT, &dead);
    CHECK(mini_key_translate(U_SPACE, 0, &dead) == '^');

    /* Una letra que no admite el acento sale sin el. */
    mini_key_translate(U_ACUTE, 0, &dead);
    CHECK(mini_key_translate(0x05, 0, &dead) == 'b');
    CHECK(dead == MINI_DEAD_NONE);

    /* El acento se consume aunque la siguiente tecla no sea una letra. */
    mini_key_translate(U_ACUTE, 0, &dead);
    CHECK(mini_key_translate(U_ENTER, 0, &dead) == TUI_KEY_ENTER);
    CHECK(mini_key_translate(U_A, 0, &dead) == 'a');

    /* Dos muertos seguidos: vale el ultimo. */
    mini_key_translate(U_ACUTE, 0, &dead);
    mini_key_translate(U_GRAVE, 0, &dead);
    CHECK(dead == MINI_DEAD_GRAVE);
}

static void test_function_and_navigation_keys(void)
{
    CHECK(press(U_ENTER, 0) == TUI_KEY_ENTER);
    CHECK(press(0x58, 0) == TUI_KEY_ENTER);
    CHECK(press(0x29, 0) == TUI_KEY_ESCAPE);
    CHECK(press(0x2a, 0) == TUI_KEY_BACKSPACE);
    CHECK(press(U_TAB, 0) == TUI_KEY_TAB);
    CHECK(press(U_TAB, MINI_MOD_LSHIFT) == TUI_KEY_BACKTAB);
    CHECK(press(U_SPACE, 0) == ' ');
    CHECK(press(0x4f, 0) == TUI_KEY_RIGHT);
    CHECK(press(0x50, 0) == TUI_KEY_LEFT);
    CHECK(press(0x51, 0) == TUI_KEY_DOWN);
    CHECK(press(0x52, 0) == TUI_KEY_UP);
    CHECK(press(0x4a, 0) == TUI_KEY_HOME);
    CHECK(press(0x4d, 0) == TUI_KEY_END);
    CHECK(press(0x4c, 0) == TUI_KEY_DELETE);
    CHECK(press(0x49, 0) == TUI_KEY_INSERT);
    CHECK(press(0x4b, 0) == TUI_KEY_PAGEUP);
    CHECK(press(0x4e, 0) == TUI_KEY_PAGEDOWN);
    CHECK(press(0x3a, 0) == TUI_KEY_F1);
    CHECK(press(0x3e, 0) == TUI_KEY_F5);
    CHECK(press(0x45, 0) == TUI_KEY_F12);
}

static void test_keypad_and_unknown_keys(void)
{
    CHECK(press(0x59, 0) == '1');
    CHECK(press(0x61, 0) == '9');
    CHECK(press(0x62, 0) == '0');
    CHECK(press(0x63, 0) == '.');
    CHECK(press(0x57, 0) == '+');
    CHECK(press(0x56, 0) == '-');
    CHECK(press(0x54, 0) == '/');
    CHECK(press(0x55, 0) == '*');
    /* Teclas sin significado para el TUI: nada, y sin efectos secundarios. */
    CHECK(press(MINI_USAGE_CAPSLOCK, 0) == TUI_KEY_NONE);
    CHECK(press(0x65, 0) == TUI_KEY_NONE);
    CHECK(press(0xff, 0) == TUI_KEY_NONE);
    CHECK(press(0x00, 0) == TUI_KEY_NONE);
}

static void test_every_letter_has_a_distinct_key(void)
{
    int usage;
    int seen_lower = 0;
    int seen_upper = 0;

    for (usage = 0x04; usage <= 0x1d; ++usage) {
        int lower = press(usage, 0);
        int upper = press(usage, MINI_MOD_LSHIFT);

        if (lower >= 'a' && lower <= 'z' && upper == lower - 'a' + 'A') {
            ++seen_lower;
            ++seen_upper;
        }
    }
    CHECK(seen_lower == 26);
    CHECK(seen_upper == 26);
}

/*
 * Un Edit acepta lo que su consola dice que es imprimible: ASCII en las
 * consolas de PC, y ademas 128..255 (CP437) en la de la MiniCPU. Sin esto la
 * enie y los acentos se descartaban aunque el teclado ya los entregara.
 */
static void test_edit_accepts_what_the_console_prints(void)
{
    char buffer[16];
    TuiDesktop desktop;
    TuiEdit edit;

    buffer[0] = '\0';
    test_init_desktop(&desktop);
    tui_edit_init(&edit, 1, 1, 10, buffer, 16);
    tui_add(&desktop.control, &edit.control);
    tui_desktop_set_focus(&desktop, &edit.control);

    /* Consola de PC: solo ASCII. */
    test_console_8bit = 0;
    CHECK(!test_key(&desktop, 0xa4));
    CHECK(edit.length == 0);

    /* Consola CP437: la enie entra, y se dibuja con su codigo. */
    test_console_8bit = 1;
    CHECK(test_key(&desktop, 'a'));
    CHECK(test_key(&desktop, 0xa4));
    CHECK(test_key(&desktop, 0x82));
    CHECK(edit.length == 3);
    CHECK((unsigned char)tui_edit_get_text(&edit)[1] == 0xa4);
    tui_draw(&desktop);
    {
        int x;
        int found_enie = -1;

        for (x = 0; x < TEST_WIDTH; ++x)
            if (test_cell_chars[1][x] == 0xa4)
                found_enie = x;
        CHECK(found_enie >= 0);
        /* Y el siguiente caracter, justo a su lado. */
        CHECK(found_enie >= 0 && test_cell_chars[1][found_enie + 1] == 0x82);
        CHECK(found_enie >= 1 && test_cell_chars[1][found_enie - 1] == 'a');
    }

    /* 127 (DEL) y los codigos de la libreria no son texto. */
    CHECK(!test_key(&desktop, 127));
    CHECK(edit.length == 3);
    test_console_8bit = 0;
}

/*
 * Partial redraw must give the same screen as a full one, while touching only
 * the cells of the region it was asked for.
 */
typedef struct {
    int chars[TEST_HEIGHT][TEST_WIDTH];
    int attrs[TEST_HEIGHT][TEST_WIDTH];
} Screen;

static void screen_save(Screen *s)
{
    memcpy(s->chars, test_cell_chars, sizeof(s->chars));
    memcpy(s->attrs, test_cell_attrs, sizeof(s->attrs));
}

static int screens_equal(const Screen *a, const Screen *b)
{
    return memcmp(a->chars, b->chars, sizeof(a->chars)) == 0 &&
           memcmp(a->attrs, b->attrs, sizeof(a->attrs)) == 0;
}

typedef struct {
    TuiDesktop desktop;
    TuiWindow left;
    TuiWindow right;
    TuiEdit edit_left;
    TuiEdit edit_right;
    TuiLabel label;
    char buffer_left[16];
    char buffer_right[16];
} Scene;

/* Two windows with an Edit each, plus a label that overlaps the left one. */
static void scene_build(Scene *s)
{
    s->buffer_left[0] = '\0';
    s->buffer_right[0] = '\0';
    test_init_desktop(&s->desktop);
    tui_window_init(&s->left, 1, 1, 24, 6, "Left");
    tui_window_init(&s->right, 30, 1, 24, 6, "Right");
    tui_edit_init(&s->edit_left, 1, 1, 20, s->buffer_left, 16);
    tui_edit_init(&s->edit_right, 1, 1, 20, s->buffer_right, 16);
    tui_label_init(&s->label, 2, 20, "Status");
    tui_add(&s->desktop.control, &s->left.control);
    tui_add(&s->desktop.control, &s->right.control);
    tui_add(&s->desktop.control, &s->label.control);
    tui_add(&s->left.control, &s->edit_left.control);
    tui_add(&s->right.control, &s->edit_right.control);
    tui_desktop_set_focus(&s->desktop, &s->edit_left.control);
}

static void test_partial_redraw_matches_full(void)
{
    Scene full;
    Scene part;
    Screen after_full;
    Screen after_partial;
    Screen before;
    int x1, y1, x2, y2;
    int x, y;
    int outside_untouched;

    /* Reference: type, then redraw everything. */
    scene_build(&full);
    tui_draw(&full.desktop);
    test_key(&full.desktop, 'h');
    test_key(&full.desktop, 'i');
    tui_draw(&full.desktop);
    screen_save(&after_full);

    /* Same keys, but redraw only the window that handled them. */
    scene_build(&part);
    tui_draw(&part.desktop);
    screen_save(&before);
    test_key(&part.desktop, 'h');
    CHECK(part.desktop.last_handler == &part.edit_left.control);
    test_key(&part.desktop, 'i');
    CHECK(tui_window_of(part.desktop.last_handler) == &part.left.control);

    tui_control_rect(&part.left.control, &x1, &y1, &x2, &y2);
    tui_draw_begin(&part.desktop);
    tui_draw_region(&part.desktop, x1, y1, x2, y2);
    tui_draw_end(&part.desktop);
    screen_save(&after_partial);

    CHECK(screens_equal(&after_full, &after_partial));

    /* And it really was partial: nothing outside the window was rewritten. */
    outside_untouched = 1;
    for (y = 0; y < TEST_HEIGHT; ++y)
        for (x = 0; x < TEST_WIDTH; ++x)
            if ((x < x1 || x >= x2 || y < y1 || y >= y2) &&
                (after_partial.chars[y][x] != before.chars[y][x] ||
                 after_partial.attrs[y][x] != before.attrs[y][x]))
                outside_untouched = 0;
    CHECK(outside_untouched);

    /* The cursor follows the edit, which was inside the region. */
    CHECK(part.desktop.cursor_visible);
}

/* Redraw only the handling control, with a window stacked over part of it. */
static void test_control_redraw_matches_full_with_overlap(void)
{
    Scene full;
    Scene part;
    TuiWindow over_full;
    TuiWindow over_part;
    Screen after_full;
    Screen after_partial;
    int x1, y1, x2, y2;

    scene_build(&full);
    tui_window_init(&over_full, 8, 2, 10, 3, "Over");
    tui_add(&full.desktop.control, &over_full.control);
    tui_draw(&full.desktop);
    test_key(&full.desktop, 'h');
    test_key(&full.desktop, 'i');
    tui_draw(&full.desktop);
    screen_save(&after_full);

    scene_build(&part);
    tui_window_init(&over_part, 8, 2, 10, 3, "Over");
    tui_add(&part.desktop.control, &over_part.control);
    tui_draw(&part.desktop);
    test_key(&part.desktop, 'h');
    test_key(&part.desktop, 'i');
    CHECK(part.desktop.last_handler == &part.edit_left.control);
    CHECK((part.edit_left.control.flags & TUI_LOCAL) != 0);

    tui_control_rect(part.desktop.last_handler, &x1, &y1, &x2, &y2);
    tui_draw_begin(&part.desktop);
    tui_draw_region(&part.desktop, x1, y1, x2, y2);
    tui_draw_end(&part.desktop);
    screen_save(&after_partial);

    CHECK(screens_equal(&after_full, &after_partial));
}

static void test_last_handler_is_cleared(void)
{
    Scene s;

    scene_build(&s);
    tui_draw(&s.desktop);
    test_key(&s.desktop, 'x');
    CHECK(s.desktop.last_handler != 0);

    /* TAB is handled by the desktop itself: no control to point at. */
    test_key(&s.desktop, TUI_KEY_TAB);
    CHECK(s.desktop.last_handler == 0);

    /* A key nobody handles leaves nothing behind either. */
    test_key(&s.desktop, TUI_KEY_F12);
    CHECK(s.desktop.last_handler == 0);
}

void test_mini_keys_suite(void)
{
    test_run_case("partial redraw equals full redraw",
                  test_partial_redraw_matches_full);
    test_run_case("control redraw equals full redraw under overlap",
                  test_control_redraw_matches_full_with_overlap);
    test_run_case("dispatch clears last handler", test_last_handler_is_cleared);
    test_run_case("edit accepts 8-bit console text",
                  test_edit_accepts_what_the_console_prints);
    test_run_case("mini keys letters", test_letters);
    test_run_case("mini keys digits", test_digits_and_shifted_digits);
    test_run_case("mini keys altgr", test_altgr_symbols);
    test_run_case("mini keys spanish symbols", test_spanish_symbols);
    test_run_case("mini keys enie and cedilla", test_enie_and_cedilla);
    test_run_case("mini keys dead keys", test_dead_keys_compose);
    test_run_case("mini keys dead key edge cases", test_dead_keys_edge_cases);
    test_run_case("mini keys function keys", test_function_and_navigation_keys);
    test_run_case("mini keys keypad and unknown", test_keypad_and_unknown_keys);
    test_run_case("mini keys full alphabet", test_every_letter_has_a_distinct_key);
}
