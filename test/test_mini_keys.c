#include <string.h>

#include "test_support.h"
#include "../mini_keys.h"
#include "../tui_internal.h"

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

/*
 * Invalidation: controls report what they changed and tui_draw_pending()
 * repaints only that. The screen must come out identical to a full redraw.
 */
static int pending_equals_full(TuiDesktop *desktop)
{
    Screen after_pending;
    Screen after_full;

    tui_draw_pending(desktop);
    screen_save(&after_pending);
    tui_draw(desktop);
    screen_save(&after_full);

    return screens_equal(&after_pending, &after_full);
}

static void test_pending_typing_under_overlap(void)
{
    Scene s;
    TuiWindow over;
    Screen before;
    Screen after;
    int x1, y1, x2, y2;
    int x, y;
    int outside_untouched;

    scene_build(&s);

    /* A window stacked over part of the edit that is about to change. */
    tui_window_init(&over, 8, 2, 10, 3, "Over");
    tui_add(&s.desktop.control, &over.control);

    tui_draw(&s.desktop);
    screen_save(&before);

    test_key(&s.desktop, 'h');
    test_key(&s.desktop, 'i');

    CHECK(!s.desktop.dirty_all);
    CHECK(s.desktop.dirty_count > 0);

    tui_draw_pending(&s.desktop);
    screen_save(&after);

    /* The window covers the typed text but is drawn again on top of it. */
    CHECK(pending_equals_full(&s.desktop));

    /* And it really was partial: nothing outside the edit changed. */
    tui_control_rect(&s.edit_left.control, &x1, &y1, &x2, &y2);
    outside_untouched = 1;
    for (y = 0; y < TEST_HEIGHT; ++y)
        for (x = 0; x < TEST_WIDTH; ++x)
            if ((x < x1 || x >= x2 || y < y1 || y >= y2) &&
                (after.chars[y][x] != before.chars[y][x] ||
                 after.attrs[y][x] != before.attrs[y][x]))
                outside_untouched = 0;
    CHECK(outside_untouched);

    /* The cursor follows the edit. */
    CHECK(s.desktop.cursor_visible);
}

static void test_pending_focus_and_tree_changes(void)
{
    Scene s;
    int i;

    scene_build(&s);
    tui_draw(&s.desktop);

    /* Moving the focus around, across windows: both ends repaint. */
    for (i = 0; i < 4; ++i) {
        test_key(&s.desktop, TUI_KEY_TAB);
        CHECK(!s.desktop.dirty_all);
        CHECK(pending_equals_full(&s.desktop));
    }

    /* Typing after the focus moved still shows the cursor. */
    test_key(&s.desktop, 'x');
    CHECK(pending_equals_full(&s.desktop));
    CHECK(s.desktop.cursor_visible);

    /* A window goes away and comes back. */
    tui_remove(&s.right.control);
    CHECK(pending_equals_full(&s.desktop));
    tui_add(&s.desktop.control, &s.right.control);
    CHECK(pending_equals_full(&s.desktop));

    /* Raising a window changes who is on top. */
    tui_bring_to_front(&s.left.control);
    CHECK(pending_equals_full(&s.desktop));

    /* A label that changes text gets shorter and the old text is erased. */
    tui_label_set_text(&s.label, "A much longer status text");
    CHECK(pending_equals_full(&s.desktop));
    tui_label_set_text(&s.label, "Ok");
    CHECK(pending_equals_full(&s.desktop));
}

typedef struct {
    TuiDesktop desktop;
    TuiWindow window;
    TuiCheckBox check;
    TuiRadioButton radio_a;
    TuiRadioButton radio_b;
    TuiListBox list;
    TuiComboBox combo;
    TuiButton button;
} Widgets;

static const char *widget_items[] = { "Apple", "Banana", "Orange", "Pear", "Plum" };

static void widgets_build(Widgets *w)
{
    test_init_desktop(&w->desktop);
    tui_window_init(&w->window, 2, 2, 40, 14, "Widgets");
    tui_checkbox_init(&w->check, 1, 1, 12, "Check");
    tui_radiobutton_init(&w->radio_a, 1, 2, 12, "One", 1);
    tui_radiobutton_init(&w->radio_b, 1, 3, 12, "Two", 1);
    tui_listbox_init(&w->list, 1, 5, 14, 3, widget_items, 5);
    tui_combobox_init(&w->combo, 18, 1, 14, widget_items, 5);
    tui_button_init(&w->button, 18, 3, 10, "Go", TUI_CMD_NONE);

    tui_add(&w->desktop.control, &w->window.control);
    tui_add(&w->window.control, &w->check.control);
    tui_add(&w->window.control, &w->radio_a.control);
    tui_add(&w->window.control, &w->radio_b.control);
    tui_add(&w->window.control, &w->list.control);
    tui_add(&w->window.control, &w->combo.control);
    tui_add(&w->window.control, &w->button.control);
}

static void test_pending_widgets(void)
{
    Widgets w;

    widgets_build(&w);
    tui_draw(&w.desktop);

    tui_desktop_set_focus(&w.desktop, &w.check.control);
    test_key(&w.desktop, ' ');
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));

    /* A radio button unchecks its sibling, which must repaint as well. */
    tui_desktop_set_focus(&w.desktop, &w.radio_a.control);
    test_key(&w.desktop, ' ');
    CHECK(pending_equals_full(&w.desktop));
    tui_desktop_set_focus(&w.desktop, &w.radio_b.control);
    test_key(&w.desktop, ' ');
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
    CHECK(!tui_radiobutton_get_checked(&w.radio_a));

    tui_desktop_set_focus(&w.desktop, &w.list.control);
    test_key(&w.desktop, TUI_KEY_DOWN);
    test_key(&w.desktop, TUI_KEY_DOWN);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
    test_key(&w.desktop, TUI_KEY_END);
    CHECK(pending_equals_full(&w.desktop));

    tui_desktop_set_focus(&w.desktop, &w.button.control);
    test_key(&w.desktop, ' ');
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));

    /* The drop-down list comes and goes; the combo shows the new choice. */
    tui_desktop_set_focus(&w.desktop, &w.combo.control);
    test_key(&w.desktop, ' ');
    CHECK(w.combo.open);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
    test_key(&w.desktop, TUI_KEY_DOWN);
    test_key(&w.desktop, TUI_KEY_DOWN);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
    test_key(&w.desktop, TUI_KEY_ENTER);
    CHECK(!w.combo.open);
    CHECK(tui_combobox_get_selected(&w.combo) == 2);
    CHECK(pending_equals_full(&w.desktop));
}

/*
 * Coming from a control in ANOTHER window the focus change alone marks both
 * frames, and then the window is raised and the list opens: that must not make
 * the desktop give up and repaint everything.
 */
static void test_pending_combo_from_another_window(void)
{
    Widgets w;
    TuiWindow other;
    TuiEdit other_edit;
    char buffer[16];

    widgets_build(&w);
    buffer[0] = '\0';
    tui_window_init(&other, 44, 2, 30, 6, "Other");
    tui_edit_init(&other_edit, 1, 1, 20, buffer, 16);
    tui_add(&w.desktop.control, &other.control);
    tui_add(&other.control, &other_edit.control);
    tui_draw(&w.desktop);

    tui_desktop_set_focus(&w.desktop, &other_edit.control);
    tui_draw_pending(&w.desktop);

    /* What a click on the combo does: focus it, raise its window, open it. */
    tui_desktop_set_focus(&w.desktop, &w.combo.control);
    tui_bring_to_front(&w.window.control);
    test_key(&w.desktop, ' ');
    CHECK(w.combo.open);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));

    /* And closing it again, then back to the other window. */
    test_key(&w.desktop, TUI_KEY_ESCAPE);
    CHECK(!w.combo.open);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
    tui_desktop_set_focus(&w.desktop, &other_edit.control);
    tui_bring_to_front(&other.control);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
}

/* Far more changes than rectangles: they merge, nothing is lost, nothing gives up. */
static void test_pending_many_changes_merge(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    TuiLabel labels[24];
    int i;

    test_init_desktop(&desktop);
    tui_window_init(&window, 2, 2, 70, 26, "Many");
    tui_add(&desktop.control, &window.control);

    for (i = 0; i < 24; ++i) {
        tui_label_init(&labels[i], 1 + (i % 4) * 16, 1 + i, "old text");
        tui_add(&window.control, &labels[i].control);
    }
    tui_draw(&desktop);

    for (i = 0; i < 24; ++i)
        tui_label_set_text(&labels[i], (i % 2) ? "x" : "a longer text");

    CHECK(!desktop.dirty_all);
    CHECK(desktop.dirty_count <= TUI_DIRTY_MAX);
    CHECK(pending_equals_full(&desktop));
}

/* Only the frame of a window changes with its flags: four thin strips, no more. */
static void test_pending_frame_only(void)
{
    Scene s;
    int i;
    int cells;
    int x1, y1, x2, y2;

    scene_build(&s);
    tui_draw(&s.desktop);

    tui_window_add_flags(&s.left, TUI_WINDOW_ACTIVE_DOUBLE);
    CHECK(!s.desktop.dirty_all);
    CHECK(s.desktop.dirty_count == 4);

    /* The ring is far smaller than the window it surrounds. */
    cells = 0;
    for (i = 0; i < s.desktop.dirty_count; ++i)
        cells += (s.desktop.dirty[i][2] - s.desktop.dirty[i][0]) *
                 (s.desktop.dirty[i][3] - s.desktop.dirty[i][1]);
    tui_control_rect(&s.left.control, &x1, &y1, &x2, &y2);
    CHECK(cells == 2 * (x2 - x1) + 2 * (y2 - y1 - 2));
    CHECK(cells < (x2 - x1) * (y2 - y1));

    CHECK(pending_equals_full(&s.desktop));
}

/* A rectangle that covers earlier ones takes their place. */
static void test_pending_big_rectangle_absorbs_small_ones(void)
{
    Scene s;
    int i;

    scene_build(&s);
    tui_draw(&s.desktop);

    /* Five cells on the diagonal of a 24x6 window, all inside it. */
    for (i = 0; i < 5; ++i)
        tui_invalidate_rect(&s.left.control, 1 + i, 1 + i, 1, 1);
    CHECK(s.desktop.dirty_count == 5);

    tui_invalidate(&s.left.control);
    CHECK(s.desktop.dirty_count == 1);
    CHECK(!s.desktop.dirty_all);
    CHECK(pending_equals_full(&s.desktop));
}

static void test_unhandled_key_draws_nothing(void)
{
    Scene s;

    scene_build(&s);
    tui_draw(&s.desktop);
    CHECK(!s.desktop.dirty_all);
    CHECK(s.desktop.dirty_count == 0);

    /* Nobody handles F12: nothing changed, nothing to repaint. */
    CHECK(!test_key(&s.desktop, TUI_KEY_F12));
    CHECK(!s.desktop.dirty_all);
    CHECK(s.desktop.dirty_count == 0);
}

static void dummy_draw(TuiControl *control, TuiDraw *draw)
{
    (void)control;
    (void)draw;
}

static int dummy_event(TuiControl *control, TuiEvent *event)
{
    (void)control;
    return event->type == TUI_EV_KEY && event->key == 'z';
}

static const TuiClass dummy_class = { dummy_draw, dummy_event };

/* A control that does not report its changes is repainted everywhere: safe. */
static void test_unreported_event_repaints_all(void)
{
    Scene s;
    TuiControl dummy;

    scene_build(&s);
    tui_control_init(&dummy, &dummy_class, 0, 0, 3, 1,
                     TUI_VISIBLE | TUI_ENABLED | TUI_FOCUSABLE);
    tui_add(&s.desktop.control, &dummy);
    tui_draw(&s.desktop);
    tui_desktop_set_focus(&s.desktop, &dummy);
    tui_draw_pending(&s.desktop);
    CHECK(!s.desktop.dirty_all);

    CHECK(test_key(&s.desktop, 'z'));
    CHECK(s.desktop.dirty_all);
}

void test_mini_keys_suite(void)
{
    test_run_case("pending redraw: typing under an overlapping window",
                  test_pending_typing_under_overlap);
    test_run_case("pending redraw: focus and tree changes",
                  test_pending_focus_and_tree_changes);
    test_run_case("pending redraw: widgets", test_pending_widgets);
    test_run_case("pending redraw: many changes merge",
                  test_pending_many_changes_merge);
    test_run_case("pending redraw: frame only", test_pending_frame_only);
    test_run_case("pending redraw: a big rectangle absorbs small ones",
                  test_pending_big_rectangle_absorbs_small_ones);
    test_run_case("pending redraw: combo opened from another window",
                  test_pending_combo_from_another_window);
    test_run_case("pending redraw: unhandled key draws nothing",
                  test_unhandled_key_draws_nothing);
    test_run_case("pending redraw: unreported event repaints all",
                  test_unreported_event_repaints_all);
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
