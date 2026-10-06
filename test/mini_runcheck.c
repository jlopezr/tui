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
 * El resultado queda en la primera fila de la pantalla: "DIFF 0000" es que esta bien.
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

    clear_rows();
    for (n = 0; n < 5; ++n)
        tui_console_cell(n, 0, "DIFF "[n], 0x07);
    tui_console_cell(5, 0, '0' + (mismatches / 1000) % 10, 0x07);
    tui_console_cell(6, 0, '0' + (mismatches / 100) % 10, 0x07);
    tui_console_cell(7, 0, '0' + (mismatches / 10) % 10, 0x07);
    tui_console_cell(8, 0, '0' + mismatches % 10, 0x07);

    return mismatches;
}
