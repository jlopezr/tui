/*
 * Comprobacion en el simulador de la MiniCPU (make mini-runcheck): los tramos de
 * celdas de console_mini.c, tui_console_fill y tui_console_text, tienen que dejar la
 * RAM de texto igual que una llamada a tui_console_cell por celda. La fila 1 se
 * escribe celda a celda, la fila 2 con el tramo, y se comparan. Las filas 3 y 4 no se
 * escriben nunca: si un tramo se sale por la derecha, cambia la 3.
 *
 * Los casos recortan por la izquierda y por la derecha, usan un caracter abstracto y
 * bytes con el bit alto (un char con signo), repiten la escritura (la copia de la
 * pantalla hace que no cambie nada) y la repiten con otro atributo.
 *
 * Ademas, el cursor y el puntero (se dibujan encima del texto): un tramo o una celda\n * que cae en su celda los oculta antes de escribir, y uno que cae lejos los deja.\n *\n * El resultado queda en la primera fila de la pantalla: "DIFF 0000" es que esta bien.
 */
#define TUI_BACKEND_MMIO
#include "../console_mini.c"

static int mismatches;

static void clear_rows(void)
{
    int x;

    for (x = 0; x < MINI_WIDTH; ++x) {
        tui_console_cell(x, 1, 'x', 0x31);
        tui_console_cell(x, 2, 'x', 0x31);
        tui_console_cell(x, 1, ' ', 0x07);
        tui_console_cell(x, 2, ' ', 0x07);
    }
}

static void compare_rows(void)
{
    int x;

    for (x = 0; x < MINI_WIDTH; ++x) {
        if (MINI_TEXT_RAM[MINI_WIDTH + x] != MINI_TEXT_RAM[2 * MINI_WIDTH + x])
            ++mismatches;

        if (MINI_TEXT_RAM[3 * MINI_WIDTH + x] != MINI_TEXT_RAM[4 * MINI_WIDTH + x])
            ++mismatches;
    }
}

static void fill_case(int x, int n, int ch, int attr)
{
    int k;

    clear_rows();
    for (k = 0; k < n; ++k)
        tui_console_cell(x + k, 1, ch, attr);
    tui_console_fill(x, 2, n, ch, attr);
    compare_rows();

    /* The same again: nothing changes, and nothing breaks. */
    tui_console_fill(x, 2, n, ch, attr);
    compare_rows();

    /* Another attribute over it. */
    for (k = 0; k < n; ++k)
        tui_console_cell(x + k, 1, ch, attr ^ 0x5a);
    tui_console_fill(x, 2, n, ch, attr ^ 0x5a);
    compare_rows();
}

static void text_case(const char *s, int x, int n, int attr)
{
    int k;

    clear_rows();
    for (k = 0; k < n; ++k)
        tui_console_cell(x + k, 1, s[k] & 0xff, attr);
    tui_console_text(x, 2, s, n, attr);
    compare_rows();

    tui_console_text(x, 2, s, n, attr);
    compare_rows();

    for (k = 0; k < n; ++k)
        tui_console_cell(x + k, 1, s[k] & 0xff, attr ^ 0x3c);
    tui_console_text(x, 2, s, n, attr ^ 0x3c);
    compare_rows();
}

static void expect(int ok)
{
    if (!ok)
        ++mismatches;
}

static int ram(int x, int y)
{
    return (int)MINI_TEXT_RAM[y * MINI_WIDTH + x];
}

/* Draws the cursor at (x, 2), or the pointer on that cell, and shows it. */
static void overlay_show(int x, int cursor, int pointer)
{
    if (cursor)
        tui_console_cursor(x, 2, 1);
    if (pointer) {
        mini_pointer_px = x * 8 + 3;
        mini_pointer_py = 2 * 16 + 5;
        mini_pointer_visible = 1;
    }
    tui_console_present();
}

static void overlay_hide(int x)
{
    tui_console_cursor(x, 2, 0);
    mini_pointer_visible = 0;
    tui_console_present();
}

/*
 * The cursor and the pointer are drawn over the text and hidden before a write
 * reaches their cell, and only then. A write that is not hidden first would leave
 * the old text to be put back when they are hidden (the "under" they saved).
 */
static void overlay_cases(void)
{
    /* The cursor, with a run over it: it is hidden, and the new text is what stays. */
    clear_rows();
    tui_console_cell(10, 2, 'a', 0x07);
    overlay_show(10, 1, 0);
    expect(mini_cursor_drawn);
    tui_console_fill(5, 2, 10, 'b', 0x1e);
    expect(!mini_cursor_drawn);
    tui_console_present();
    overlay_hide(10);
    expect(ram(10, 2) == (int)mini_cell('b', 0x1e));

    /* The first and the last cell of the run are inside it; the ones next to it are not. */
    clear_rows();
    overlay_show(14, 1, 0);
    tui_console_fill(5, 2, 10, 'd', 0x1e);
    expect(!mini_cursor_drawn);
    overlay_hide(14);
    expect(ram(14, 2) == (int)mini_cell('d', 0x1e));

    clear_rows();
    overlay_show(5, 1, 0);
    tui_console_text(5, 2, "efghijklmn", 10, 0x1e);
    expect(!mini_cursor_drawn);
    overlay_hide(5);
    expect(ram(5, 2) == (int)mini_cell('e', 0x1e));

    clear_rows();
    overlay_show(15, 1, 0);
    tui_console_fill(5, 2, 10, 'd', 0x1e);
    expect(mini_cursor_drawn);
    overlay_hide(15);
    expect(ram(15, 2) == (int)mini_cell(' ', 0x07));

    clear_rows();
    overlay_show(4, 1, 0);
    tui_console_text(5, 2, "efghijklmn", 10, 0x1e);
    expect(mini_cursor_drawn);
    overlay_hide(4);
    expect(ram(4, 2) == (int)mini_cell(' ', 0x07));

    /* The pointer on the last cell of a run. */
    clear_rows();
    overlay_show(14, 0, 1);
    tui_console_fill(5, 2, 10, 'd', 0x1e);
    expect(!mini_pointer_drawn);
    overlay_hide(14);
    expect(ram(14, 2) == (int)mini_cell('d', 0x1e));

    /* The cursor, with a run beside it: it is left alone. */
    clear_rows();
    overlay_show(30, 1, 0);
    tui_console_fill(5, 2, 10, 'b', 0x1e);
    tui_console_text(40, 2, "text", 4, 0x1e);
    expect(mini_cursor_drawn);
    overlay_hide(30);
    expect(ram(30, 2) == (int)mini_cell(' ', 0x07));

    /* The same for the pointer, with text. */
    clear_rows();
    overlay_show(20, 0, 1);
    expect(mini_pointer_drawn);
    tui_console_text(18, 2, "pqrst", 5, 0x2e);
    expect(!mini_pointer_drawn);
    tui_console_present();
    overlay_hide(20);
    expect(ram(20, 2) == (int)mini_cell('r', 0x2e));

    clear_rows();
    overlay_show(20, 0, 1);
    tui_console_fill(25, 2, 5, 'q', 0x2e);
    expect(mini_pointer_drawn);
    overlay_hide(20);
    expect(ram(20, 2) == (int)mini_cell(' ', 0x07));

    /* Both on the same cell, under a run: both hidden, in the usual order. */
    clear_rows();
    overlay_show(12, 1, 1);
    expect(mini_cursor_drawn && mini_pointer_drawn);
    tui_console_fill(8, 2, 10, 'c', 0x4f);
    expect(!mini_cursor_drawn && !mini_pointer_drawn);
    tui_console_present();
    overlay_hide(12);
    expect(ram(12, 2) == (int)mini_cell('c', 0x4f));

    /* A single cell under the pointer. */
    clear_rows();
    overlay_show(40, 0, 1);
    tui_console_cell(40, 2, 'z', 0x4f);
    expect(!mini_pointer_drawn);
    tui_console_present();
    overlay_hide(40);
    expect(ram(40, 2) == (int)mini_cell('z', 0x4f));
}

int main(void)
{
    static char s[8];
    int n;

    tui_console_init();

    s[0] = 'h';
    s[1] = 'e';
    s[2] = (char)0xa4;
    s[3] = 'l';
    s[4] = 'o';
    s[5] = (char)0xe9;
    s[6] = ' ';
    s[7] = (char)0xff;

    fill_case(0, 80, 'a', 0x1e);
    fill_case(5, 10, TUI_CH_HLINE, 0x07);
    fill_case(-5, 10, 'b', 0x70);
    fill_case(75, 10, 'c', 0x0f);
    fill_case(40, 0, 'd', 0x07);
    fill_case(78, 2, TUI_CH_TL, 0x4f);
    fill_case(10, 5, TUI_CH_SCROLL_THUMB, 0x1f);
    fill_case(79, 1, ' ', 0x07);

    text_case(s, 0, 8, 0x1e);
    text_case(s, -3, 8, 0x70);
    text_case(s, 76, 8, 0x0f);
    text_case(s, 10, 0, 0x07);
    text_case(s, 20, 8, 0x07);

    overlay_cases();

    clear_rows();
    for (n = 0; n < 5; ++n)
        tui_console_cell(n, 0, "DIFF "[n], 0x07);
    tui_console_cell(5, 0, '0' + (mismatches / 1000) % 10, 0x07);
    tui_console_cell(6, 0, '0' + (mismatches / 100) % 10, 0x07);
    tui_console_cell(7, 0, '0' + (mismatches / 10) % 10, 0x07);
    tui_console_cell(8, 0, '0' + mismatches % 10, 0x07);

    return mismatches;
}
