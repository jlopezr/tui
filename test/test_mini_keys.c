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

/* Cells written by the partial repaint that follows one event. */
static int cells_for_key(Widgets *w, TuiControl *focus, int key)
{
    int before;

    tui_desktop_set_focus(&w->desktop, focus);
    tui_draw_pending(&w->desktop);
    test_key(&w->desktop, key);
    before = test_cells_written;
    tui_draw_pending(&w->desktop);

    return test_cells_written - before;
}

/*
 * A control that paints its whole rectangle is drawn without what is behind it
 * (the window and the desktop), so the repaint writes about the cells of the
 * control itself: a 12-cell check box takes 20 (its fill, then its glyphs and
 * text), where it took 44 with the desktop and the window drawn first.
 */
static void test_pending_cost(void)
{
    Widgets w;

    widgets_build(&w);
    tui_draw(&w.desktop);

    CHECK(cells_for_key(&w, &w.check.control, ' ') <= 24);
    CHECK(cells_for_key(&w, &w.radio_b.control, ' ') <= 24);
    CHECK(cells_for_key(&w, &w.list.control, TUI_KEY_DOWN) <= 110);
}

/*
 * Starting from the opaque control that covers the region must not lose what is
 * drawn after it: a label over an edit, and a window over part of both.
 */
static void test_pending_from_the_cover(void)
{
    TuiDesktop desktop;
    TuiWindow back;
    TuiWindow front;
    TuiEdit edit;
    TuiLabel label;
    static char buffer[32] = "some text";

    test_init_desktop(&desktop);
    tui_window_init(&back, 2, 2, 40, 8, "Back");
    tui_window_init(&front, 20, 4, 30, 8, "Front");
    tui_edit_init(&edit, 1, 1, 24, buffer, 32);
    tui_label_init(&label, 4, 1, "label");

    tui_add(&desktop.control, &back.control);
    tui_add(&back.control, &edit.control);
    tui_add(&back.control, &label.control);
    tui_add(&desktop.control, &front.control);
    tui_draw(&desktop);

    /* A label over an edit: the edit is the cover, the label is drawn after it. */
    tui_invalidate(&label.control);
    CHECK(pending_equals_full(&desktop));

    /* The edit, partly under the front window. */
    tui_invalidate(&edit.control);
    CHECK(pending_equals_full(&desktop));

    /* The border of the back window, and the front window's own frame. */
    tui_invalidate_frame(&back.control);
    CHECK(pending_equals_full(&desktop));
    tui_invalidate_frame(&front.control);
    CHECK(pending_equals_full(&desktop));

    /* A rectangle that crosses the border: no child covers it, the window does. */
    tui_invalidate_rect(&back.control, 0, 0, 5, 3);
    CHECK(pending_equals_full(&desktop));
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

    /* The arrow of the combo points up while its list is open, and back down. */
    {
        int x1, y1, x2, y2;

        tui_control_rect(&w.combo.control, &x1, &y1, &x2, &y2);
        CHECK(test_cell_chars[y1][x2 - 2] == TUI_CH_UP_TRIANGLE);
    }
    test_key(&w.desktop, TUI_KEY_DOWN);
    test_key(&w.desktop, TUI_KEY_DOWN);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));
    test_key(&w.desktop, TUI_KEY_ENTER);
    CHECK(!w.combo.open);
    CHECK(tui_combobox_get_selected(&w.combo) == 2);
    CHECK(pending_equals_full(&w.desktop));
    {
        int x1, y1, x2, y2;

        tui_control_rect(&w.combo.control, &x1, &y1, &x2, &y2);
        CHECK(test_cell_chars[y1][x2 - 2] == TUI_CH_DOWN_TRIANGLE);
    }
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

/* The flags of a window only change its frame: one entry of the frame list. */
static void test_pending_frame_only(void)
{
    Scene s;
    Screen before;
    Screen after;
    int x, y;
    int x1, y1, x2, y2;
    int inside_untouched;

    scene_build(&s);
    tui_draw(&s.desktop);
    screen_save(&before);

    tui_window_add_flags(&s.left, TUI_WINDOW_ACTIVE_DOUBLE);
    CHECK(!s.desktop.dirty_all);
    CHECK(s.desktop.dirty_count == 0);
    CHECK(s.desktop.frame_count == 1);

    tui_draw_pending(&s.desktop);
    screen_save(&after);

    /* The inside of the window (the edit, the background) was not rewritten. */
    tui_control_rect(&s.left.control, &x1, &y1, &x2, &y2);
    inside_untouched = 1;
    for (y = y1 + 1; y < y2 - 1; ++y)
        for (x = x1 + 1; x < x2 - 1; ++x)
            if (after.chars[y][x] != before.chars[y][x] ||
                after.attrs[y][x] != before.attrs[y][x])
                inside_untouched = 0;
    CHECK(inside_untouched);

    CHECK(pending_equals_full(&s.desktop));
}

/*
 * Moving the focus to another window: two controls and, if the windows are drawn
 * differently when active, two frames; four entries.
 */
static void test_pending_focus_across_windows_uses_four_entries(void)
{
    Scene s;

    scene_build(&s);
    tui_window_add_flags(&s.left, TUI_WINDOW_ACTIVE_DOUBLE);
    tui_window_add_flags(&s.right, TUI_WINDOW_ACTIVE_DOUBLE);
    tui_draw(&s.desktop);

    tui_desktop_set_focus(&s.desktop, &s.edit_right.control);
    CHECK(s.desktop.dirty_count == 2);
    CHECK(s.desktop.frame_count == 2);
    CHECK(!s.desktop.dirty_all);
    CHECK(pending_equals_full(&s.desktop));
}

/* Windows that look the same active or not: their frames are not repainted. */
static void test_pending_focus_across_plain_windows_skips_frames(void)
{
    Scene s;

    scene_build(&s);
    tui_draw(&s.desktop);

    tui_desktop_set_focus(&s.desktop, &s.edit_right.control);
    CHECK(s.desktop.dirty_count == 2);
    CHECK(s.desktop.frame_count == 0);
    CHECK(pending_equals_full(&s.desktop));
}

/* A rectangle over a frame absorbs it; a frame inside a rectangle is not listed. */
static void test_pending_frames_and_rectangles_absorb_each_other(void)
{
    Scene s;

    scene_build(&s);
    tui_draw(&s.desktop);

    tui_invalidate_frame(&s.left.control);
    tui_invalidate_frame(&s.left.control);              /* the same frame twice */
    CHECK(s.desktop.frame_count == 1);
    tui_invalidate(&s.left.control);
    CHECK(s.desktop.frame_count == 0);
    CHECK(s.desktop.dirty_count == 1);

    tui_invalidate_frame(&s.left.control);              /* already inside a rectangle */
    CHECK(s.desktop.frame_count == 0);
    CHECK(pending_equals_full(&s.desktop));
}

/* More frames than the list holds: the extra ones become rectangles. */
static void test_pending_many_frames_overflow_to_rectangles(void)
{
    TuiDesktop desktop;
    TuiWindow windows[TUI_FRAME_MAX + 4];
    int i;

    test_init_desktop(&desktop);
    for (i = 0; i < TUI_FRAME_MAX + 4; ++i) {
        tui_window_init(&windows[i], 2 + (i % 6) * 12, 1 + (i / 6) * 8, 10, 6, "w");
        tui_add(&desktop.control, &windows[i].control);
    }
    tui_draw(&desktop);

    for (i = 0; i < TUI_FRAME_MAX + 4; ++i)
        tui_window_add_flags(&windows[i], TUI_WINDOW_ACTIVE_DOUBLE);

    CHECK(desktop.frame_count == TUI_FRAME_MAX);
    CHECK(desktop.dirty_count == 4);
    CHECK(!desktop.dirty_all);
    CHECK(pending_equals_full(&desktop));
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

/*
 * Rectangles that overlap are one rectangle (the overlap is not painted twice),
 * unless their union is much bigger than the two together.
 */
static void test_pending_overlapping_rectangles_merge(void)
{
    Scene s;

    scene_build(&s);
    tui_draw(&s.desktop);

    /* A window dragged two cells: old and new place are one rectangle. */
    tui_invalidate_rect(&s.left.control, 0, 0, 10, 4);
    tui_invalidate_rect(&s.left.control, 2, 0, 10, 4);
    CHECK(s.desktop.dirty_count == 1);
    CHECK(s.desktop.dirty[0][2] - s.desktop.dirty[0][0] == 12);
    CHECK(s.desktop.dirty[0][3] - s.desktop.dirty[0][1] == 4);
    CHECK(pending_equals_full(&s.desktop));

    /* They only touch at a corner: the union (18 x 7) is bigger than both. */
    tui_invalidate_rect(&s.left.control, 0, 0, 10, 4);
    tui_invalidate_rect(&s.left.control, 8, 3, 10, 4);
    CHECK(s.desktop.dirty_count == 2);
    CHECK(pending_equals_full(&s.desktop));

    /* Next to each other, without overlap: they stay apart. */
    tui_invalidate_rect(&s.left.control, 0, 0, 5, 4);
    tui_invalidate_rect(&s.left.control, 5, 0, 5, 4);
    CHECK(s.desktop.dirty_count == 2);
    CHECK(pending_equals_full(&s.desktop));

    /* A chain: the third reaches both, and the three become one. */
    tui_invalidate_rect(&s.left.control, 0, 0, 6, 4);
    tui_invalidate_rect(&s.left.control, 12, 0, 6, 4);
    CHECK(s.desktop.dirty_count == 2);
    tui_invalidate_rect(&s.left.control, 4, 0, 10, 4);
    CHECK(s.desktop.dirty_count == 1);
    CHECK(pending_equals_full(&s.desktop));
}

/* Dragging a window across another: one rectangle per step, not two. */
static void test_pending_window_drag_is_one_rectangle(void)
{
    Scene s;
    TuiEvent event;
    int i;

    scene_build(&s);
    tui_draw(&s.desktop);

    /* Press the title of the left window and move it right, a cell at a time. */
    test_mouse_down(&s.desktop, 5, 1);
    CHECK(s.left.dragging);
    tui_draw_pending(&s.desktop);

    for (i = 1; i <= 6; ++i) {
        test_mouse_action(&s.desktop, 5 + i, 1, TUI_MOUSE_MOVE, &event);
        CHECK(!s.desktop.dirty_all);
        CHECK(s.desktop.dirty_count == 1);
        CHECK(pending_equals_full(&s.desktop));
    }
}

/*
 * Dragging a window against the edge of its parent: it can go half out, but not
 * away; what is outside is clipped, so nothing outside the parent changes at any
 * step (not even for an instant), each step looks like a full redraw, and the title
 * can always be grabbed again.
 */
static void drag_out_of_parent(int docked, int dx, int dy, int steps)
{
    int cx1, cy1, cx2, cy2;
    int left_edge;
    int right_edge;
    int hit_x;

    TuiDesktop desktop;
    TuiPanel screen;
    TuiWindow left;
    TuiWindow right;
    TuiWindow parent;
    TuiWindow child;
    TuiWindow sibling;
    TuiWindow below;
    TuiEvent event;
    Screen before;
    Screen now;
    Screen full;
    int px1, py1, px2, py2;
    int sx, sy;
    int x, y;
    int i;
    int outside_untouched;

    test_init_desktop(&desktop);
    tui_window_init(&child, 2, 2, 12, 5, "Child");
    tui_window_init(&sibling, 4, 5, 16, 6, "Sibling");

    if (docked) {
        /* As in the demo: a panel with a window on each side and one filling the rest. */
        tui_panel_init(&screen, 0, 0, 1, 1);
        screen.control.dock = TUI_DOCK_FILL;
        tui_window_init(&left, 0, 0, 12, 5, "Left");
        left.control.dock = TUI_DOCK_LEFT;
        tui_window_init(&right, 0, 0, 15, 5, "Right");
        right.control.dock = TUI_DOCK_RIGHT;
        tui_window_init(&parent, 0, 0, 1, 1, "Workspace");
        parent.control.dock = TUI_DOCK_FILL;
        tui_add(&parent.control, &child.control);
        tui_add(&parent.control, &sibling.control);
        tui_add(&screen.control, &left.control);
        tui_add(&screen.control, &right.control);
        tui_add(&screen.control, &parent.control);
        tui_add(&desktop.control, &screen.control);
        sx = 12 + 1 + 2 + 3;
        sy = 0 + 1 + 2;
    } else {
        tui_window_init(&below, 0, 0, 60, 20, "Below");
        tui_window_init(&parent, 10, 3, 30, 12, "Parent");
        tui_add(&desktop.control, &below.control);
        tui_add(&desktop.control, &parent.control);
        tui_add(&parent.control, &child.control);
        tui_add(&parent.control, &sibling.control);
        sx = 10 + 1 + 2 + 3;
        sy = 3 + 1 + 2;
    }

    tui_draw(&desktop);
    screen_save(&before);
    tui_control_rect(&parent.control, &px1, &py1, &px2, &py2);

    /* Press the title of the child. */
    test_mouse_down(&desktop, sx, sy);
    CHECK(child.dragging);
    tui_draw_pending(&desktop);

    for (i = 1; i <= steps; ++i) {
        test_mouse_action(&desktop, sx + i * dx, sy + i * dy, TUI_MOUSE_MOVE, &event);
        CHECK(!desktop.dirty_all);
        test_watch_outside(px1, py1, px2, py2);
        tui_draw_pending(&desktop);
        test_watch_off();
        screen_save(&now);

        /* Not even for an instant: what is outside the parent is not touched. */
        CHECK(test_watch_writes == 0);

        outside_untouched = 1;
        for (y = 0; y < TEST_HEIGHT; ++y)
            for (x = 0; x < TEST_WIDTH; ++x)
                if ((x < px1 || x >= px2 || y < py1 || y >= py2) &&
                    (now.chars[y][x] != before.chars[y][x] ||
                     now.attrs[y][x] != before.attrs[y][x]))
                    outside_untouched = 0;
        CHECK(outside_untouched);

        tui_draw(&desktop);
        screen_save(&full);
        CHECK(screens_equal(&now, &full));

        /* Four cells of the title inside the parent, on a row it shows. */
        tui_control_rect(&child.control, &cx1, &cy1, &cx2, &cy2);
        left_edge = cx1 > px1 + 1 ? cx1 : px1 + 1;
        right_edge = cx2 < px2 - 1 ? cx2 : px2 - 1;
        CHECK(cy1 > py1 && cy1 < py2 - 1);
        CHECK(right_edge - left_edge >= 4);

        /* And a click there finds the window, which is on top. */
        hit_x = left_edge + 1;
        CHECK(tui_hit_test(&desktop.control, hit_x, cy1) == &child.control);
    }
}

static void test_pending_window_drag_against_the_parent(void)
{
    drag_out_of_parent(0, 1, 0, 30);    /* the right edge */
    drag_out_of_parent(0, -1, 0, 20);   /* the left edge */
    drag_out_of_parent(0, 0, 1, 14);    /* the bottom */
    drag_out_of_parent(0, 0, -1, 6);    /* the top */

    drag_out_of_parent(1, 1, 0, 60);    /* towards the Right window */
    drag_out_of_parent(1, -1, 0, 30);   /* towards the Left window */
    drag_out_of_parent(1, 0, 1, 25);    /* down, towards the status line */
    drag_out_of_parent(1, 0, -1, 4);    /* up */
}

/*
 * The editor tells the desktop what an event changed, not its whole rectangle:
 * moving the cursor writes a couple of cells, typing rewrites a row, Enter the rows
 * below it. Each step must still look exactly like a full redraw.
 */
typedef struct EditorScene {
    TuiDesktop desktop;
    TuiWindow window;
    TuiLinearTextModel model;
    TuiEditor editor;
    char buffer[2048];
} EditorScene;

/* A window at (x, y) holding an editor of w x h cells (and a border around it). */
static void editor_scene_build(EditorScene *s, int x, int y, int w, int h,
                               int lines, int length)
{
    char text[128];
    int i;
    int j;

    test_init_desktop(&s->desktop);
    s->buffer[0] = '\0';
    tui_linear_text_model_init(&s->model, s->buffer, (int)sizeof(s->buffer));

    for (i = 0; i < lines; ++i) {
        for (j = 0; j < length + (i % 3) * 9 && j < 100; ++j)
            text[j] = (char)('a' + (i + j) % 26);

        text[j++] = '\n';
        tui_text_model_insert(&s->model.model,
                              tui_text_model_length(&s->model.model), text, j);
    }

    tui_window_init(&s->window, x, y, w + 2, h + 2, "Editor");
    tui_editor_init(&s->editor, 0, 0, w, h, &s->model.model);
    tui_add(&s->window.control, &s->editor.control);
    tui_add(&s->desktop.control, &s->window.control);
    tui_desktop_set_focus(&s->desktop, &s->editor.control);
    tui_draw(&s->desktop);
}

/* The cursor the console was left with by the last paint_and_check(). */
static int checked_cursor_visible;
static int checked_cursor_x;
static int checked_cursor_y;

/* Paints what is pending, checks it against a full redraw, returns the cells it wrote. */
static int paint_and_check(TuiDesktop *desktop)
{
    Screen now;
    Screen full;
    int before;
    int written;

    before = test_cells_written;
    tui_draw_pending(desktop);
    written = test_cells_written - before;
    checked_cursor_visible = test_cursor_visible;
    checked_cursor_x = test_cursor_x;
    checked_cursor_y = test_cursor_y;
    screen_save(&now);

    tui_draw(desktop);
    screen_save(&full);
    CHECK(screens_equal(&now, &full));

    return written;
}

static int editor_paint_and_check(EditorScene *s)
{
    return paint_and_check(&s->desktop);
}

static void test_pending_editor_writes_only_what_changed(void)
{
    static EditorScene s;
    int written;
    int i;

    /* The editor of the demo's Editor screen: 78 x 23 inside an 80 x 25 window. */
    editor_scene_build(&s, 0, 0, 78, 23, 12, 40);

    /* The cursor moves: the cell it is now on. */
    test_key(&s.desktop, TUI_KEY_DOWN);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written >= 1 && written <= 4);

    test_key(&s.desktop, TUI_KEY_RIGHT);
    written = editor_paint_and_check(&s);
    CHECK(written >= 1 && written <= 4);

    /* A click in the text: the same. */
    test_mouse_down(&s.desktop, 1 + 12, 1 + 6);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written >= 1 && written <= 4);

    /* A key that changes nothing (Up on the first line): nothing is repainted. */
    for (i = 0; i < 12; ++i)
        test_key(&s.desktop, TUI_KEY_UP);
    test_key(&s.desktop, TUI_KEY_HOME);
    editor_paint_and_check(&s);

    test_key(&s.desktop, TUI_KEY_UP);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written == 0);

    /*
     * A letter in the middle of a line: that row, from the cursor on. Each cell is
     * written twice at most (the background, then the character).
     */
    test_key(&s.desktop, TUI_KEY_DOWN);
    test_key(&s.desktop, TUI_KEY_RIGHT);
    editor_paint_and_check(&s);
    test_key(&s.desktop, 'z');
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written >= 2 && written <= 2 * 78);

    test_key(&s.desktop, TUI_KEY_BACKSPACE);
    written = editor_paint_and_check(&s);
    CHECK(written >= 2 && written <= 2 * 78);

    /* Enter: the rows from the cursor's down, not the rows above. */
    test_key(&s.desktop, TUI_KEY_ENTER);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written >= 78 && written < 78 * 23);
}

/*
 * The focus moving to and from a big control draws what changes with it: for an
 * editor, the cell the cursor is on, and the cursor itself comes and goes.
 */
static void test_pending_focus_draws_only_what_changes(void)
{
    static EditorScene s;
    static TuiButton button;
    int written;
    int i;

    /* An editor of 78 x 21 and a button below it, both in the same window. */
    editor_scene_build(&s, 0, 0, 78, 21, 12, 40);
    s.window.control.height = 25;
    tui_button_init(&button, 2, 22, 12, "OK", 77);
    tui_add(&s.window.control, &button.control);
    tui_draw(&s.desktop);

    for (i = 0; i < 3; ++i)
        test_key(&s.desktop, TUI_KEY_DOWN);
    for (i = 0; i < 5; ++i)
        test_key(&s.desktop, TUI_KEY_RIGHT);
    editor_paint_and_check(&s);
    CHECK(checked_cursor_visible);
    CHECK(checked_cursor_x == 1 + 5 && checked_cursor_y == 1 + 3);

    /* Away from the editor: its cursor cell, and the button. Not the whole editor. */
    tui_desktop_set_focus(&s.desktop, &button.control);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written >= 1 && written <= 60);
    CHECK(!checked_cursor_visible);

    /* Back to it: the cursor is where it was. */
    tui_desktop_set_focus(&s.desktop, &s.editor.control);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written >= 1 && written <= 60);
    CHECK(checked_cursor_visible);
    CHECK(checked_cursor_x == 1 + 5 && checked_cursor_y == 1 + 3);

    /*
     * To nothing at all: the cursor cell, and the window's frame, which is no longer
     * the active one: the ring of cells around an 80 x 25 window (206), more than
     * once over, as what lies under it is drawn first. Far from the 1800 of the
     * editor, or the 2000 of the whole window.
     */
    tui_desktop_set_focus(&s.desktop, 0);
    written = editor_paint_and_check(&s);
    CHECK(written <= 500);
    CHECK(!checked_cursor_visible);

    /*
     * A read-only editor shows no cursor, so nothing of it changes with the focus:
     * what is drawn is the button, in the same window (no frame either).
     */
    tui_desktop_set_focus(&s.desktop, &button.control);
    editor_paint_and_check(&s);
    tui_editor_set_readonly(&s.editor, 1);
    editor_paint_and_check(&s);
    tui_desktop_set_focus(&s.desktop, &s.editor.control);
    CHECK(!s.desktop.dirty_all);
    written = editor_paint_and_check(&s);
    CHECK(written <= 40);
    CHECK(!checked_cursor_visible);
}

/* A list drawn differently with the focus: only its selected row changes. */
static void test_pending_listbox_focus_draws_one_row(void)
{
    static const char *items[] = {
        "Apple", "Banana", "Orange", "Peach", "Pear", "Cherry",
        "Lemon", "Mango", "Plum", "Fig", "Lime", "Date"
    };
    TuiDesktop desktop;
    TuiWindow window;
    TuiListBox list;
    TuiButton button;
    int written;

    test_init_desktop(&desktop);
    tui_window_init(&window, 2, 2, 40, 14, "List");
    tui_listbox_init(&list, 0, 0, 20, 8, items, 12);
    tui_button_init(&button, 2, 10, 10, "OK", 78);
    tui_add(&window.control, &list.control);
    tui_add(&window.control, &button.control);
    tui_add(&desktop.control, &window.control);
    tui_listbox_set_selected(&list, 5);
    tui_desktop_set_focus(&desktop, &button.control);
    tui_draw(&desktop);

    /* The selected row, and the button: not the eight rows. */
    tui_desktop_set_focus(&desktop, &list.control);
    CHECK(!desktop.dirty_all);
    written = paint_and_check(&desktop);
    CHECK(written >= 1 && written <= 2 * 20 + 40);

    tui_desktop_set_focus(&desktop, &button.control);
    written = paint_and_check(&desktop);
    CHECK(written >= 1 && written <= 2 * 20 + 40);

    /* A selection that has scrolled out of view: nothing in the list to repaint. */
    tui_listbox_set_selected(&list, 11);
    paint_and_check(&desktop);
    tui_desktop_set_focus(&desktop, &list.control);
    CHECK(!desktop.dirty_all);
    paint_and_check(&desktop);
}

/*
 * The focus moving between two windows with an Editor over a buffer each (the demo's "TextArea"
 * and "Scrolling"): the two frames, which show which window is active, and the
 * cursor cell of each text area. Not the text areas themselves.
 */
static void test_pending_editor_buffer_focus_draws_only_the_cursor(void)
{
    static TuiDesktop desktop;
    static TuiWindow w1, w2;
    static TuiEditor a1, a2;
    static char b1[64], b2[64];
    int written;
    int i;

    test_init_desktop(&desktop);
    tui_window_init(&w1, 0, 16, 25, 5, "TextArea");
    tui_window_init(&w2, 26, 16, 25, 5, "Scrolling");
    strcpy(b1, "Some text\nsecond line\nthird\nfourth");
    strcpy(b2, "Other text\nline two\nline three\nfour");
    tui_editor_init_buffer(&a1, 0, 0, 23, 3, b1, 64);
    tui_editor_init_buffer(&a2, 0, 0, 23, 3, b2, 64);
    tui_add(&w1.control, &a1.control);
    tui_add(&w2.control, &a2.control);
    tui_add(&desktop.control, &w1.control);
    tui_add(&desktop.control, &w2.control);
    tui_desktop_set_focus(&desktop, &a1.control);
    tui_draw(&desktop);

    for (i = 0; i < 4; ++i) {
        tui_desktop_set_focus(&desktop, i % 2 == 0 ? &a2.control : &a1.control);
        CHECK(!desktop.dirty_all);
        written = paint_and_check(&desktop);
        /* These windows look the same active or not: just the two cursor cells. */
        CHECK(written >= 1 && written <= 8);
        CHECK(checked_cursor_visible);
    }

    /* Drawn double when active, the frames change too: the ring, and the title. */
    tui_window_add_flags(&w1, TUI_WINDOW_ACTIVE_DOUBLE);
    tui_window_add_flags(&w2, TUI_WINDOW_ACTIVE_DOUBLE);
    paint_and_check(&desktop);
    tui_desktop_set_focus(&desktop, &a2.control);
    written = paint_and_check(&desktop);
    CHECK(written >= 100 && written <= 2 * (56 + 12) + 20);

    /* A read-only one has no cursor: nothing of it is drawn. */
    tui_editor_set_readonly(&a2, 1);
    paint_and_check(&desktop);
    tui_desktop_set_focus(&desktop, &a2.control);
    paint_and_check(&desktop);
    CHECK(!checked_cursor_visible);
}

/* A window with an Edit, a ListBox with a scroll bar, and a Button. */
typedef struct FineScene {
    TuiDesktop desktop;
    TuiWindow window;
    TuiEdit edit;
    TuiListBox list;
    TuiButton button;
    char buffer[40];
} FineScene;

static void fine_scene_build(FineScene *s)
{
    static const char *items[] = {
        "Apple", "Banana", "Orange", "Peach", "Pear", "Cherry",
        "Lemon", "Mango", "Plum", "Fig", "Lime", "Date"
    };

    test_init_desktop(&s->desktop);
    strcpy(s->buffer, "The quick brown fox jumps over it");
    tui_window_init(&s->window, 2, 2, 44, 14, "Fine");
    tui_edit_init(&s->edit, 0, 0, 20, s->buffer, 40);
    tui_listbox_init(&s->list, 0, 2, 20, 8, items, 12);
    tui_button_init(&s->button, 24, 2, 10, "OK", 78);
    tui_add(&s->window.control, &s->edit.control);
    tui_add(&s->window.control, &s->list.control);
    tui_add(&s->window.control, &s->button.control);
    tui_add(&s->desktop.control, &s->window.control);
    tui_desktop_set_focus(&s->desktop, &s->list.control);
    tui_draw(&s->desktop);
}

/* Moving the selection of a list repaints the two rows, or all of it if it scrolls. */
static void test_pending_listbox_moves_draw_two_rows(void)
{
    static FineScene s;
    int written;
    int i;

    fine_scene_build(&s);

    test_key(&s.desktop, TUI_KEY_DOWN);
    CHECK(!s.desktop.dirty_all);
    written = paint_and_check(&s.desktop);
    /* Two rows, each with the background, the selection bar and the text. */
    CHECK(written >= 2 && written <= 2 * (2 * 19 + 8));

    /* A click on a row. */
    test_mouse_down(&s.desktop, 3 + 2, 5 + 5);
    CHECK(!s.desktop.dirty_all);
    written = paint_and_check(&s.desktop);
    CHECK(written >= 2 && written <= 2 * (2 * 19 + 8));

    /* A click on the row that is already selected, and a key that changes nothing. */
    test_mouse_down(&s.desktop, 3 + 2, 5 + 5);
    CHECK(!s.desktop.dirty_all);
    written = paint_and_check(&s.desktop);
    CHECK(written == 0);
    tui_listbox_set_selected(&s.list, 0);
    paint_and_check(&s.desktop);
    test_key(&s.desktop, TUI_KEY_UP);
    written = paint_and_check(&s.desktop);
    CHECK(written == 0);
    test_key(&s.desktop, '#');
    written = paint_and_check(&s.desktop);
    CHECK(written == 0);

    /* Going past the last visible row scrolls: the whole list. */
    for (i = 0; i < 9; ++i)
        test_key(&s.desktop, TUI_KEY_DOWN);
    CHECK(s.list.offset > 0);
    paint_and_check(&s.desktop);
}

/* Typing in an Edit repaints from the column that changed; moving, one cell. */
static void test_pending_edit_draws_what_changed(void)
{
    static FineScene s;
    int written;
    int i;

    fine_scene_build(&s);
    tui_desktop_set_focus(&s.desktop, &s.edit.control);
    paint_and_check(&s.desktop);

    test_key(&s.desktop, TUI_KEY_HOME);
    CHECK(!s.desktop.dirty_all);
    paint_and_check(&s.desktop);
    CHECK(checked_cursor_visible);
    test_key(&s.desktop, TUI_KEY_RIGHT);
    written = paint_and_check(&s.desktop);
    CHECK(written >= 1 && written <= 4);
    CHECK(checked_cursor_x == 3 + 1);

    /* Nothing changes: nothing is painted. */
    test_key(&s.desktop, TUI_KEY_HOME);
    paint_and_check(&s.desktop);
    test_key(&s.desktop, TUI_KEY_LEFT);
    CHECK(!s.desktop.dirty_all);
    written = paint_and_check(&s.desktop);
    CHECK(written == 0);

    /* A letter: from the cursor to the right edge, not the cells before it. */
    for (i = 0; i < 10; ++i)
        test_key(&s.desktop, TUI_KEY_RIGHT);
    paint_and_check(&s.desktop);
    test_key(&s.desktop, 'z');
    CHECK(!s.desktop.dirty_all);
    written = paint_and_check(&s.desktop);
    CHECK(written >= 2 && written <= 2 * 10);

    test_key(&s.desktop, TUI_KEY_BACKSPACE);
    written = paint_and_check(&s.desktop);
    CHECK(written >= 2 && written <= 2 * 11);

    /* A click in the text. */
    test_mouse_down(&s.desktop, 3 + 4, 3);
    CHECK(!s.desktop.dirty_all);
    written = paint_and_check(&s.desktop);
    CHECK(written >= 1 && written <= 4);

    /* The text scrolls: it is drawn whole. */
    test_key(&s.desktop, TUI_KEY_END);
    CHECK(s.edit.offset > 0);
    paint_and_check(&s.desktop);

    /* The focus: the cursor cell only. */
    tui_desktop_set_focus(&s.desktop, &s.button.control);
    CHECK(!s.desktop.dirty_all);
    paint_and_check(&s.desktop);
    CHECK(!checked_cursor_visible);
    tui_desktop_set_focus(&s.desktop, &s.edit.control);
    paint_and_check(&s.desktop);
    CHECK(checked_cursor_visible);
}

/* The Edit and the ListBox under random events: always what a full redraw gives. */
static void test_pending_edit_and_listbox_random_events(void)
{
    static FineScene s;
    unsigned int seed;
    int step;
    int r;

    fine_scene_build(&s);
    seed = 987u;

    for (step = 0; step < 1500; ++step) {
        seed = seed * 1103515245u + 12345u;
        r = (int)((seed >> 16) & 0x7fff) % 22;

        if (r < 6) {
            static const int keys[] = {
                TUI_KEY_UP, TUI_KEY_DOWN, TUI_KEY_LEFT, TUI_KEY_RIGHT,
                TUI_KEY_HOME, TUI_KEY_END
            };
            test_key(&s.desktop, keys[r]);
        } else if (r < 8) {
            test_key(&s.desktop, r == 6 ? TUI_KEY_PAGEUP : TUI_KEY_PAGEDOWN);
        } else if (r < 10) {
            test_key(&s.desktop, r == 8 ? TUI_KEY_BACKSPACE : TUI_KEY_DELETE);
        } else if (r < 14) {
            test_key(&s.desktop, 'a' + (int)((seed >> 8) % 26u));
        } else if (r < 16) {
            test_key(&s.desktop, TUI_KEY_TAB);
        } else if (r < 18) {
            test_key(&s.desktop, TUI_KEY_ENTER);
        } else {
            test_mouse_down(&s.desktop, 3 + (int)((seed >> 4) % 22u),
                            3 + (int)((seed >> 12) % 11u));
        }

        paint_and_check(&s.desktop);
    }
}

/*
 * Repainting what is already on the screen changes no cell. The text of a row used
 * to be drawn over a fill of spaces, so each letter's cell was changed twice (and
 * the MiniCPU console writes both to the text RAM).
 */
static int changed_by_repaint(TuiDesktop *desktop, TuiControl *control)
{
    int before;
    int changed;

    paint_and_check(desktop);
    before = test_cells_changed;
    tui_invalidate(control);
    tui_draw_pending(desktop);
    changed = test_cells_changed - before;

    /* Still the same screen as a full redraw. */
    paint_and_check(desktop);

    return changed;
}

static void test_pending_unchanged_repaint_changes_no_cell(void)
{
    static EditorScene s;
    static FineScene f;
    int written;
    int before;

    editor_scene_build(&s, 0, 0, 78, 23, 12, 40);
    CHECK(changed_by_repaint(&s.desktop, &s.editor.control) == 0);

    /* Scrolled, so the first line is not the first row; and with the cursor moved. */
    test_key(&s.desktop, TUI_KEY_PAGEDOWN);
    test_key(&s.desktop, TUI_KEY_RIGHT);
    CHECK(changed_by_repaint(&s.desktop, &s.editor.control) == 0);

    /* Each cell of the editor is written once: 78 x 23 and nothing more. */
    paint_and_check(&s.desktop);
    before = test_cells_written;
    tui_invalidate(&s.editor.control);
    tui_draw_pending(&s.desktop);
    written = test_cells_written - before;
    CHECK(written <= 78 * 23 + 2 * 23);

    fine_scene_build(&f);
    tui_desktop_set_focus(&f.desktop, &f.edit.control);
    CHECK(changed_by_repaint(&f.desktop, &f.edit.control) == 0);
    CHECK(changed_by_repaint(&f.desktop, &f.list.control) == 0);

    /* The selected row, with and without the focus. */
    tui_desktop_set_focus(&f.desktop, &f.list.control);
    test_key(&f.desktop, TUI_KEY_DOWN);
    CHECK(changed_by_repaint(&f.desktop, &f.list.control) == 0);
}

/* A small editor with both scroll bars and a long text, hit with random events. */
static void test_pending_editor_random_events(void)
{
    static EditorScene s;
    static TuiButton button;
    unsigned int seed;
    int step;
    int r;
    int key;

    editor_scene_build(&s, 2, 2, 30, 8, 14, 22);
    seed = 12345u;

    /* A button under the editor to move the focus to, and back from. */
    s.window.control.height = 13;
    tui_button_init(&button, 2, 10, 10, "OK", 77);
    tui_add(&s.window.control, &button.control);
    tui_draw(&s.desktop);

    for (step = 0; step < 1500; ++step) {
        seed = seed * 1103515245u + 12345u;
        r = (int)((seed >> 16) & 0x7fff);

        if (r % 23 == 0)
            tui_desktop_set_focus(&s.desktop,
                                  tui_desktop_get_focus(&s.desktop) == &button.control
                                      ? &s.editor.control : &button.control);

        switch (r % 20) {
        case 8:  key = ' '; break;
        case 9:  key = TUI_KEY_ENTER; break;
        case 10: key = TUI_KEY_BACKSPACE; break;
        case 11: key = TUI_KEY_DELETE; break;
        case 12: key = TUI_KEY_LEFT; break;
        case 13: key = TUI_KEY_RIGHT; break;
        case 14: key = TUI_KEY_UP; break;
        case 15: key = TUI_KEY_DOWN; break;
        case 16: key = TUI_KEY_HOME; break;
        case 17: key = TUI_KEY_END; break;
        case 18: key = (r / 20) % 2 ? TUI_KEY_PAGEUP : TUI_KEY_PAGEDOWN; break;
        case 19: key = -1; break;
        default: key = 'a' + (r / 20) % 26; break;
        }

        if (key < 0) {
            /* A click anywhere on the editor, scroll bar strips included. */
            test_mouse_down(&s.desktop,
                            3 + (r / 20) % 31,
                            3 + (r / 640) % 9);
        } else {
            test_key(&s.desktop, key);
        }

        editor_paint_and_check(&s);
    }

    /* Whatever happened, it was all editing and moving, and the screen agrees. */
    CHECK(tui_text_model_length(&s.model.model) > 0);
}

/* A panel is a container with no frame: it groups, docks, and draws nothing. */
static void test_panel_groups_without_a_frame(void)
{
    TuiDesktop desktop;
    TuiPanel panel;
    TuiWindow side;
    TuiWindow main_window;
    TuiEdit edit;
    char buffer[16];
    Screen background;
    Screen shown;
    int x1, y1, x2, y2;

    test_init_desktop(&desktop);
    buffer[0] = '\0';
    tui_panel_init(&panel, 0, 0, 1, 1);
    panel.control.dock = TUI_DOCK_FILL;
    tui_window_init(&side, 0, 0, 10, 5, "Side");
    side.control.dock = TUI_DOCK_LEFT;
    tui_window_init(&main_window, 0, 0, 1, 1, "Main");
    main_window.control.dock = TUI_DOCK_FILL;
    tui_edit_init(&edit, 1, 1, 12, buffer, 16);

    /* Nothing in it: the panel leaves the desktop as it was. */
    tui_draw(&desktop);
    screen_save(&background);
    tui_add(&desktop.control, &panel.control);
    CHECK(pending_equals_full(&desktop));
    screen_save(&shown);
    CHECK(screens_equal(&background, &shown));

    tui_add(&panel.control, &side.control);
    tui_add(&panel.control, &main_window.control);
    tui_add(&main_window.control, &edit.control);
    tui_desktop_set_focus(&desktop, &edit.control);
    tui_draw(&desktop);

    /* It takes the whole desktop, and docks its children with no border offset. */
    tui_control_rect(&panel.control, &x1, &y1, &x2, &y2);
    CHECK(x1 == 0 && y1 == 0 && x2 == TEST_WIDTH && y2 == TEST_HEIGHT);
    tui_control_rect(&side.control, &x1, &y1, &x2, &y2);
    CHECK(x1 == 0 && y1 == 0 && x2 == 10);
    tui_control_rect(&main_window.control, &x1, &y1, &x2, &y2);
    CHECK(x1 == 10 && y1 == 0 && x2 == TEST_WIDTH && y2 == TEST_HEIGHT);

    /* The mouse reaches what is inside it. */
    tui_desktop_set_focus(&desktop, 0);
    tui_control_rect(&edit.control, &x1, &y1, &x2, &y2);
    CHECK(test_mouse_down(&desktop, x1 + 1, y1));
    CHECK(tui_desktop_get_focus(&desktop) == &edit.control);

    /* Taking the panel out takes everything, and lets go of the focus inside it. */
    tui_remove(&panel.control);
    CHECK(tui_desktop_get_focus(&desktop) == 0);
    CHECK(pending_equals_full(&desktop));
    screen_save(&shown);
    CHECK(screens_equal(&background, &shown));

    /* And it comes back as it was. */
    tui_add(&desktop.control, &panel.control);
    CHECK(pending_equals_full(&desktop));
    tui_control_rect(&side.control, &x1, &y1, &x2, &y2);
    CHECK(x1 == 0 && x2 == 10);
}

/*
 * The console turns the second of two quick clicks on a cell into DOUBLE. A
 * control that only knows about presses must not lose that click.
 */
static void test_double_click_is_also_a_press(void)
{
    static TuiMenuItem menu_items[] = { { "Run", 84, TUI_KEY_NONE, 0 } };
    static TuiMenu menus[] = { { "File", menu_items, 1 } };
    Widgets w;
    TuiMenuBar menubar;
    TuiEvent event;
    int x1, y1, x2, y2;
    int px1, py1, px2, py2;

    widgets_build(&w);
    tui_menubar_init(&menubar, menus, 1);
    tui_add(&w.desktop.control, &menubar.control);
    tui_draw(&w.desktop);

    /* A check box: the second click of a quick pair toggles it as well. */
    tui_control_rect(&w.check.control, &x1, &y1, &x2, &y2);
    CHECK(!tui_checkbox_get_checked(&w.check));
    CHECK(test_mouse_action(&w.desktop, x1 + 1, y1, TUI_MOUSE_DOUBLE, &event));
    CHECK(tui_checkbox_get_checked(&w.check));
    CHECK(pending_equals_full(&w.desktop));

    /* A button: the double click presses it and the release activates it. */
    tui_control_rect(&w.button.control, &x1, &y1, &x2, &y2);
    CHECK(test_mouse_action(&w.desktop, x1 + 1, y1, TUI_MOUSE_DOUBLE, &event));
    CHECK(w.button.pressed);
    CHECK(test_mouse_action(&w.desktop, x1 + 1, y1, TUI_MOUSE_UP, &event));
    CHECK(event.type == TUI_EV_COMMAND);

    /* A menu title: DOUBLE opens the menu, and on the same title closes it. */
    CHECK(test_mouse_action(&w.desktop, 2, 0, TUI_MOUSE_DOUBLE, &event));
    CHECK(menubar.active);
    CHECK(test_mouse_action(&w.desktop, 2, 0, TUI_MOUSE_DOUBLE, &event));
    CHECK(!menubar.active);

    /* A combo box: DOUBLE opens it; DOUBLE on an item chooses it and closes. */
    tui_control_rect(&w.combo.control, &x1, &y1, &x2, &y2);
    CHECK(test_mouse_action(&w.desktop, x1 + 1, y1, TUI_MOUSE_DOUBLE, &event));
    CHECK(w.combo.open);
    tui_control_rect(&w.combo.popup_window.control, &px1, &py1, &px2, &py2);
    CHECK(test_mouse_action(&w.desktop, px1 + 2, py1 + 2, TUI_MOUSE_DOUBLE, &event));
    CHECK(!w.combo.open);
    CHECK(tui_combobox_get_selected(&w.combo) == 1);
    CHECK(pending_equals_full(&w.desktop));

    /* ... and DOUBLE outside the list closes it without choosing. */
    CHECK(test_mouse_action(&w.desktop, x1 + 1, y1, TUI_MOUSE_DOUBLE, &event));
    CHECK(w.combo.open);
    CHECK(test_mouse_action(&w.desktop, 70, 20, TUI_MOUSE_DOUBLE, &event));
    CHECK(!w.combo.open);
    CHECK(tui_combobox_get_selected(&w.combo) == 1);
    CHECK(pending_equals_full(&w.desktop));
}

/*
 * Menus say what they change: the bar's row and the popup's rectangle, never the
 * whole screen, and what is on screen afterwards is what a full repaint would draw.
 * The popup is over a window, so closing it has to put that window back.
 */
#define MENU_STEP_KEY(k) \
    do { \
        CHECK(test_key(&w.desktop, (k))); \
        CHECK(!w.desktop.dirty_all); \
        CHECK(pending_equals_full(&w.desktop)); \
    } while (0)

#define MENU_STEP_MOUSE(x, y, action) \
    do { \
        test_mouse_action(&w.desktop, (x), (y), (action), &event); \
        CHECK(!w.desktop.dirty_all); \
        CHECK(pending_equals_full(&w.desktop)); \
    } while (0)

static void test_pending_menus(void)
{
    static TuiMenuItem file_items[] = {
        { "New", 81, TUI_KEY_NONE, 0 },
        { "-", 0, TUI_KEY_NONE, TUI_MENU_SEPARATOR },
        { "Open", 82, TUI_KEY_NONE, 0 },
        { "Quit", 83, TUI_KEY_NONE, 0 }
    };
    static TuiMenuItem edit_items[] = {
        { "Copy", 91, TUI_KEY_NONE, 0 },
        { "Paste a long entry", 92, TUI_KEY_NONE, 0 }
    };
    static TuiMenu menus[] = {
        { "File", file_items, 4 },
        { "Edit", edit_items, 2 }
    };
    Widgets w;
    TuiMenuBar menubar;
    TuiEvent event;

    widgets_build(&w);
    tui_menubar_init(&menubar, menus, 2);
    tui_add(&w.desktop.control, &menubar.control);
    tui_draw(&w.desktop);

    /* With the keyboard: activate, open, move, change menu, leave. */
    MENU_STEP_KEY(TUI_KEY_F10);
    CHECK(menubar.active);
    MENU_STEP_KEY(TUI_KEY_DOWN);
    CHECK(menubar.popup.control.parent != 0);

    /* Moving the highlight touches two rows of the popup, not the whole popup. */
    CHECK(test_key(&w.desktop, TUI_KEY_DOWN));
    CHECK(w.desktop.dirty_count >= 1 && w.desktop.dirty_count <= 2);
    CHECK(w.desktop.dirty[0][3] - w.desktop.dirty[0][1] <= 2 ||
          w.desktop.dirty_count == 2);
    CHECK(pending_equals_full(&w.desktop));

    MENU_STEP_KEY(TUI_KEY_DOWN);
    MENU_STEP_KEY(TUI_KEY_UP);
    MENU_STEP_KEY(TUI_KEY_RIGHT);
    MENU_STEP_KEY(TUI_KEY_LEFT);
    MENU_STEP_KEY(TUI_KEY_ESCAPE);
    CHECK(!menubar.active);
    CHECK(menubar.popup.control.parent == 0);

    /* Enter on an item closes the menu and turns into a command. */
    MENU_STEP_KEY(TUI_KEY_F10);
    MENU_STEP_KEY(TUI_KEY_DOWN);
    MENU_STEP_KEY(TUI_KEY_ENTER);
    CHECK(!menubar.active);

    /* With the mouse: press a title, hover over the items and the other title. */
    MENU_STEP_MOUSE(2, 0, TUI_MOUSE_DOWN);
    CHECK(menubar.active);
    MENU_STEP_MOUSE(4, 2, TUI_MOUSE_MOVE);
    MENU_STEP_MOUSE(4, 4, TUI_MOUSE_MOVE);
    MENU_STEP_MOUSE(8, 0, TUI_MOUSE_MOVE);
    MENU_STEP_MOUSE(60, 20, TUI_MOUSE_DOWN);
    CHECK(!menubar.active);
}

/*
 * Taking a window out of the tree lets go of everything it was holding: the focus
 * and the capture, an open list (a window of the desktop, not of this window), a
 * drag and a press in progress.
 */
static void test_remove_releases_what_the_subtree_holds(void)
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

    /* An open list, a window being dragged, a pressed button, a scroll bar held. */
    tui_desktop_set_focus(&w.desktop, &w.combo.control);
    test_key(&w.desktop, ' ');
    CHECK(w.combo.open);
    CHECK(w.desktop.capture == &w.combo.control);
    w.window.dragging = 1;
    w.button.pressed = 1;
    w.list.scrollbar.dragging = 1;
    tui_draw_pending(&w.desktop);

    tui_remove(&w.window.control);

    CHECK(!w.combo.open);
    CHECK(w.combo.popup_window.control.parent == 0);
    CHECK(w.desktop.capture == 0);
    CHECK(tui_desktop_get_focus(&w.desktop) == 0);
    CHECK(w.window.dragging == 0);
    CHECK(w.button.pressed == 0);
    CHECK(w.list.scrollbar.dragging == 0);
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));

    /* It comes back as new and works. */
    tui_add(&w.desktop.control, &w.window.control);
    tui_desktop_set_focus(&w.desktop, &w.combo.control);
    test_key(&w.desktop, ' ');
    CHECK(w.combo.open);
    CHECK(pending_equals_full(&w.desktop));
    tui_combobox_close(&w.combo);

    /* The focus belongs to somebody else: removing this window leaves it alone. */
    tui_desktop_set_focus(&w.desktop, &other_edit.control);
    tui_remove(&w.window.control);
    CHECK(tui_desktop_get_focus(&w.desktop) == &other_edit.control);
    CHECK(pending_equals_full(&w.desktop));
}

/* Closing the list from outside cancels, like Esc, and repaints what it covered. */
static void test_combobox_close_cancels(void)
{
    Widgets w;

    widgets_build(&w);
    tui_draw(&w.desktop);

    /* Closed already: nothing to do and nothing to repaint. */
    tui_combobox_close(&w.combo);
    CHECK(!w.combo.open);
    CHECK(!w.desktop.dirty_all);
    CHECK(w.desktop.dirty_count == 0 && w.desktop.frame_count == 0);

    tui_desktop_set_focus(&w.desktop, &w.combo.control);
    test_key(&w.desktop, ' ');
    test_key(&w.desktop, TUI_KEY_DOWN);
    test_key(&w.desktop, TUI_KEY_DOWN);
    CHECK(w.combo.open);
    CHECK(w.desktop.capture == &w.combo.control);
    CHECK(w.combo.popup_window.control.parent != 0);
    tui_draw_pending(&w.desktop);

    tui_combobox_close(&w.combo);
    CHECK(!w.combo.open);
    CHECK(w.desktop.capture == 0);
    CHECK(w.combo.popup_window.control.parent == 0);
    CHECK(tui_combobox_get_selected(&w.combo) == 0);     /* nothing was chosen */
    CHECK(!w.desktop.dirty_all);
    CHECK(pending_equals_full(&w.desktop));

    tui_combobox_close(0);                                /* harmless */
}

static int command_ran = 0;

/* Whether the text of a screen row contains "text" (the 8-bit chars are plain here). */
static int row_has(int row, const char *text)
{
    int x;
    int k;

    for (x = 0; x < TEST_WIDTH; ++x) {
        for (k = 0; text[k] != '\0' && x + k < TEST_WIDTH; ++k) {
            if (test_cell_chars[row][x + k] != text[k])
                break;
        }
        if (text[k] == '\0')
            return 1;
    }

    return 0;
}

static void command_action(void *context, int command)
{
    *(int *)context += command;
    command_ran = command;
}

static void test_command_table_lookup(void)
{
    static const TuiCommand table[] = {
        { 10, "ten",    0 },
        { 20, 0,        command_action },
        { 30, "thirty", command_action },
        TUI_COMMANDS_END,
        { 40, "after the end", 0 }
    };
    const TuiCommand *entry;

    entry = tui_command_find(table, 10);
    CHECK(entry == &table[0]);
    CHECK(entry->text != 0 && entry->run == 0);

    entry = tui_command_find(table, 20);
    CHECK(entry == &table[1] && entry->text == 0 && entry->run != 0);

    CHECK(tui_command_find(table, 30) == &table[2]);

    /* Not there, past the end marker, none at all, and the marker itself. */
    CHECK(tui_command_find(table, 99) == 0);
    CHECK(tui_command_find(table, 40) == 0);
    CHECK(tui_command_find(0, 10) == 0);
    CHECK(tui_command_find(table, TUI_CMD_NONE) == 0);
}

/* A button's command goes through the table by itself, in tui_dispatch(). */
static void test_commands_run_by_dispatch(void)
{
    static const TuiCommand table[] = {
        { 7, "seven pressed", command_action },
        { 8, "just text",     0 },
        TUI_COMMANDS_END
    };
    TuiDesktop desktop;
    TuiWindow window;
    TuiButton seven;
    TuiButton eight;
    TuiButton unknown;
    TuiStatusBar status;
    TuiStatusItem items[1];
    int context;
    int x1, y1, x2, y2;

    items[0].text = "F1";
    items[0].key = TUI_KEY_F1;
    items[0].command = TUI_CMD_NONE;

    test_init_desktop(&desktop);
    tui_window_init(&window, 1, 1, 30, 8, "Commands");
    tui_button_init(&seven, 1, 1, 10, "Seven", 7);
    tui_button_init(&eight, 1, 2, 10, "Eight", 8);
    tui_button_init(&unknown, 1, 3, 10, "Nine", 9);
    tui_statusbar_init(&status, items, 1);
    tui_statusbar_set_text(&status, "Ready");
    tui_add(&desktop.control, &window.control);
    tui_add(&window.control, &seven.control);
    tui_add(&window.control, &eight.control);
    tui_add(&window.control, &unknown.control);
    tui_add(&desktop.control, &status.control);
    tui_draw(&desktop);

    context = 100;
    command_ran = 0;
    tui_desktop_set_commands(&desktop, table, &context, &status);

    /* Text goes to the status bar, the action gets the context, and it repaints it. */
    tui_desktop_set_focus(&desktop, &seven.control);
    tui_draw_pending(&desktop);
    CHECK(test_key(&desktop, ' '));
    CHECK(context == 107 && command_ran == 7);
    CHECK(!desktop.dirty_all);
    CHECK(pending_equals_full(&desktop));
    tui_control_rect(&status.control, &x1, &y1, &x2, &y2);
    CHECK(row_has(y1, "seven pressed"));

    /* An entry with only text does not call anything. */
    tui_desktop_set_focus(&desktop, &eight.control);
    CHECK(test_key(&desktop, ' '));
    CHECK(context == 107);
    CHECK(pending_equals_full(&desktop));
    CHECK(row_has(y1, "just text"));

    /* A command that is not in the table is ignored, and the event still says so. */
    tui_desktop_set_focus(&desktop, &unknown.control);
    CHECK(test_key(&desktop, ' '));
    CHECK(context == 107);
    CHECK(row_has(y1, "just text"));                  /* unchanged */

    /* Without a status bar the text is simply not shown. */
    tui_desktop_set_commands(&desktop, table, &context, 0);
    tui_desktop_set_focus(&desktop, &seven.control);
    CHECK(test_key(&desktop, ' '));
    CHECK(context == 114);

    /* Without a table nothing runs, as before. */
    tui_desktop_set_commands(&desktop, 0, 0, 0);
    CHECK(test_key(&desktop, ' '));
    CHECK(context == 114);

    /* And by hand. */
    tui_desktop_set_commands(&desktop, table, &context, &status);
    CHECK(tui_command_run(&desktop, 7));
    CHECK(context == 121);
    CHECK(!tui_command_run(&desktop, 99));
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
    test_run_case("pending redraw: focus across windows uses four entries",
                  test_pending_focus_across_windows_uses_four_entries);
    test_run_case("pending redraw: frames and rectangles absorb each other",
                  test_pending_frames_and_rectangles_absorb_each_other);
    test_run_case("pending redraw: frame list overflow becomes rectangles",
                  test_pending_many_frames_overflow_to_rectangles);
    test_run_case("pending redraw: a big rectangle absorbs small ones",
                  test_pending_big_rectangle_absorbs_small_ones);
    test_run_case("pending redraw: overlapping rectangles merge",
                  test_pending_overlapping_rectangles_merge);
    test_run_case("pending redraw: a window drag is one rectangle",
                  test_pending_window_drag_is_one_rectangle);
    test_run_case("pending redraw: a window dragged against its parent",
                  test_pending_window_drag_against_the_parent);
    test_run_case("pending redraw: the editor writes only what changed",
                  test_pending_editor_writes_only_what_changed);
    test_run_case("pending redraw: focus draws only what changes",
                  test_pending_focus_draws_only_what_changes);
    test_run_case("pending redraw: list focus draws one row",
                  test_pending_listbox_focus_draws_one_row);
    test_run_case("pending redraw: plain windows skip their frames on focus",
                  test_pending_focus_across_plain_windows_skips_frames);
    test_run_case("pending redraw: editor buffer focus draws only the cursor",
                  test_pending_editor_buffer_focus_draws_only_the_cursor);
    test_run_case("pending redraw: repainting what is shown changes no cell",
                  test_pending_unchanged_repaint_changes_no_cell);
    test_run_case("pending redraw: list moves draw two rows",
                  test_pending_listbox_moves_draw_two_rows);
    test_run_case("pending redraw: an edit draws what changed",
                  test_pending_edit_draws_what_changed);
    test_run_case("pending redraw: edit and list under random events",
                  test_pending_edit_and_listbox_random_events);
    test_run_case("pending redraw: the editor under random events",
                  test_pending_editor_random_events);
    test_run_case("pending redraw: combo opened from another window",
                  test_pending_combo_from_another_window);
    test_run_case("panel groups children without a frame",
                  test_panel_groups_without_a_frame);
    test_run_case("a double click is also a press",
                  test_double_click_is_also_a_press);
    test_run_case("pending redraw: menus", test_pending_menus);
    test_run_case("pending redraw: cost in cells", test_pending_cost);
    test_run_case("pending redraw: starting from the cover",
                  test_pending_from_the_cover);
    test_run_case("removing a window releases what it holds",
                  test_remove_releases_what_the_subtree_holds);
    test_run_case("combo box closed from outside cancels",
                  test_combobox_close_cancels);
    test_run_case("command table lookup", test_command_table_lookup);
    test_run_case("commands run by tui_dispatch", test_commands_run_by_dispatch);
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
