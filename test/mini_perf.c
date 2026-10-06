/*
 * Medida en la PLACA de cuatro formas de escribir tramos de celdas en la RAM de texto
 * (make mini-perf): el programa se mide a si mismo con los contadores de CPU
 * PERFORMANCE (mmio.md 13.2) y escribe el resultado en la pantalla, que se lee con
 * `monitor.py screen`. No sirve en el simulador (no tiene esos contadores).
 *
 *   A  como esta ahora: cada celda se compara con mini_shadow y, si cambia, se escribe
 *   B  como los PC: se escribe en una tabla pendiente y al presentar se compara con
 *      otra que refleja la pantalla (dos tablas)
 *   C  como B, pero al presentar se compara con la RAM de texto, leida (una tabla)
 *   D  sin tabla: cada celda se escribe siempre
 *
 * Tres situaciones, 62 fotogramas de 64 celdas cada una:
 *   1  se repinta lo mismo que ya hay
 *   2  se repinta algo distinto cada vez
 *   3  se rellena de espacios y se escribe el texto encima, con el mismo resultado
 *      que ya habia (como hacian los controles antes del punto 22)
 *
 * Cada fila del resultado: nombre, y por celda ciclos, instrucciones, esperas a
 * memoria y esperas a MMIO, en decimas ("123" es 12,3).
 */
#define TUI_BACKEND_MMIO
#include "../console_mini.c"

#define PERF_CYCLES     (*(volatile unsigned int *)0x81010000)
#define PERF_RETIRED    (*(volatile unsigned int *)0x81010004)
#define PERF_STALL_MEM  (*(volatile unsigned int *)0x81010014)
#define PERF_STALL_MMIO (*(volatile unsigned int *)0x8101001c)

#define PASSES 62
#define RUN    64
#define CELLS  (PASSES * RUN)
#define ROW    5

typedef void (*FillFn)(int x, int y, int n, int ch, int attr);
typedef void (*TextFn)(int x, int y, const char *text, int n, int attr);
typedef void (*PresentFn)(void);

static int pend[MINI_CELLS];     /* lo que han escrito los tramos del fotograma */
static int mirror[MINI_CELLS];   /* lo que tiene la pantalla (variante B) */
static int dirty_lo;
static int dirty_hi;
static char text_a[RUN + 1];
static char text_b[RUN + 1];

static void nop_present(void)
{
}

static void nop_fill(int x, int y, int n, int ch, int attr)
{
    (void)x;
    (void)y;
    (void)n;
    (void)ch;
    (void)attr;
}

static void nop_text(int x, int y, const char *text, int n, int attr)
{
    (void)x;
    (void)y;
    (void)text;
    (void)n;
    (void)attr;
}

/* B y C: el tramo solo anota en la tabla pendiente, ya con el valor del hardware. */
static void pend_fill(int x, int y, int n, int ch, int attr)
{
    int index;
    int last;
    int value;

    if (y < 0 || y >= MINI_HEIGHT)
        return;
    if (x < 0) {
        n += x;
        x = 0;
    }
    if (x + n > MINI_WIDTH)
        n = MINI_WIDTH - x;
    if (n <= 0)
        return;

    index = y * MINI_WIDTH + x;
    last = index + n - 1;
    value = (int)mini_cell(mini_glyph(ch), attr);

    if (index < dirty_lo)
        dirty_lo = index;
    if (last > dirty_hi)
        dirty_hi = last;

    for (; n > 0; --n, ++index)
        pend[index] = value;
}

static void pend_text(int x, int y, const char *text, int n, int attr)
{
    int index;
    int last;
    int base;

    if (y < 0 || y >= MINI_HEIGHT)
        return;
    if (x < 0) {
        text -= x;
        n += x;
        x = 0;
    }
    if (x + n > MINI_WIDTH)
        n = MINI_WIDTH - x;
    if (n <= 0)
        return;

    index = y * MINI_WIDTH + x;
    last = index + n - 1;
    base = (int)mini_cell(0, attr);

    if (index < dirty_lo)
        dirty_lo = index;
    if (last > dirty_hi)
        dirty_hi = last;

    for (; n > 0; --n, ++index, ++text)
        pend[index] = base | (*text & 0xff);
}

/* B: se compara con la tabla que refleja la pantalla. */
static void present_two_tables(void)
{
    int i;
    int ready;

    ready = 0;

    for (i = dirty_lo; i <= dirty_hi; ++i) {
        if (pend[i] == mirror[i])
            continue;
        if (!ready) {
            mini_overlay_hide();
            ready = 1;
        }
        mirror[i] = pend[i];
        MINI_TEXT_RAM[i] = (unsigned int)pend[i];
    }

    dirty_lo = MINI_CELLS;
    dirty_hi = -1;
}

/* C: se compara con la RAM de texto. */
static void present_read_ram(void)
{
    int i;
    int ready;

    ready = 0;

    for (i = dirty_lo; i <= dirty_hi; ++i) {
        if ((int)MINI_TEXT_RAM[i] == pend[i])
            continue;
        if (!ready) {
            mini_overlay_hide();
            ready = 1;
        }
        MINI_TEXT_RAM[i] = (unsigned int)pend[i];
    }

    dirty_lo = MINI_CELLS;
    dirty_hi = -1;
}

/* D: sin tabla ni comparacion. */
static void raw_fill(int x, int y, int n, int ch, int attr)
{
    int index;
    unsigned int value;

    if (y < 0 || y >= MINI_HEIGHT)
        return;
    if (x < 0) {
        n += x;
        x = 0;
    }
    if (x + n > MINI_WIDTH)
        n = MINI_WIDTH - x;
    if (n <= 0)
        return;

    mini_overlay_hide();
    index = y * MINI_WIDTH + x;
    value = mini_cell(mini_glyph(ch), attr);

    for (; n > 0; --n, ++index)
        MINI_TEXT_RAM[index] = value;
}

static void raw_text(int x, int y, const char *text, int n, int attr)
{
    int index;
    unsigned int base;

    if (y < 0 || y >= MINI_HEIGHT)
        return;
    if (x < 0) {
        text -= x;
        n += x;
        x = 0;
    }
    if (x + n > MINI_WIDTH)
        n = MINI_WIDTH - x;
    if (n <= 0)
        return;

    mini_overlay_hide();
    index = y * MINI_WIDTH + x;
    base = mini_cell(0, attr);

    for (; n > 0; --n, ++index, ++text)
        MINI_TEXT_RAM[index] = base | (unsigned int)(*text & 0xff);
}

static void scenario(int s, FillFn fill, TextFn text, PresentFn present)
{
    int j;

    for (j = 0; j < PASSES; ++j) {
        if (s == 1) {
            text(0, ROW, text_a, RUN, 7);
        } else if (s == 2) {
            text(0, ROW, (j & 1) ? text_b : text_a, RUN, 7);
        } else {
            fill(0, ROW, RUN, ' ', 7);
            text(0, ROW, text_a, RUN, 7);
        }
        present();
    }
}

/* The row starts blank and every table agrees with the screen. */
static void reset_row(void)
{
    int x;
    int index;

    for (x = 0; x < MINI_WIDTH; ++x)
        tui_console_cell(x, ROW, 'q', 0x1f);
    for (x = 0; x < MINI_WIDTH; ++x)
        tui_console_cell(x, ROW, ' ', 0x07);

    /* The first frame of scenario 1 and 3 puts the text; start with it there. */
    tui_console_text(0, ROW, text_a, RUN, 7);

    for (index = 0; index < MINI_CELLS; ++index) {
        pend[index] = (int)MINI_TEXT_RAM[index];
        mirror[index] = pend[index];
    }

    dirty_lo = MINI_CELLS;
    dirty_hi = -1;
}

static void put_text(int col, int row, const char *s)
{
    for (; *s != '\0'; ++s, ++col)
        tui_console_cell(col, row, *s & 0xff, 0x07);
}

/* Unsigned number in a field of 'width' columns. */
static int put_number(int col, int row, unsigned int value, int width)
{
    int i;
    int c;

    for (i = width - 1; i >= 0; --i) {
        c = (value > 0 || i == width - 1) ? '0' + (int)(value % 10u) : ' ';
        tui_console_cell(col + i, row, c, 0x07);
        value /= 10u;
    }

    return col + width;
}

static void measure(int row, const char *name, int s,
                    FillFn fill, TextFn text, PresentFn present)
{
    unsigned int c0;
    unsigned int r0;
    unsigned int m0;
    unsigned int i0;
    unsigned int c1;
    unsigned int r1;
    unsigned int m1;
    unsigned int i1;
    int col;

    reset_row();

    c0 = PERF_CYCLES;
    r0 = PERF_RETIRED;
    m0 = PERF_STALL_MEM;
    i0 = PERF_STALL_MMIO;
    scenario(s, fill, text, present);
    c1 = PERF_CYCLES;
    r1 = PERF_RETIRED;
    m1 = PERF_STALL_MEM;
    i1 = PERF_STALL_MMIO;

    reset_row();
    put_text(0, row, name);
    col = 10;
    col = put_number(col, row, (c1 - c0) * 10u / CELLS, 6);
    col = put_number(col + 1, row, (r1 - r0) * 10u / CELLS, 6);
    col = put_number(col + 1, row, (m1 - m0) * 10u / CELLS, 6);
    put_number(col + 1, row, (i1 - i0) * 10u / CELLS, 6);
}

int main(void)
{
    int i;
    int row;

    tui_console_init();

    for (i = 0; i < RUN; ++i) {
        text_a[i] = 'a' + (i % 26);
        text_b[i] = 'A' + (i % 26);
    }
    text_a[RUN] = '\0';
    text_b[RUN] = '\0';

    /* Only to have the column titles; the measures overwrite their own rows. */
    row = 10;
    measure(row++, "none 1", 1, nop_fill, nop_text, nop_present);
    measure(row++, "A 1 same", 1, tui_console_fill, tui_console_text, nop_present);
    measure(row++, "B 1 same", 1, pend_fill, pend_text, present_two_tables);
    measure(row++, "C 1 same", 1, pend_fill, pend_text, present_read_ram);
    measure(row++, "D 1 same", 1, raw_fill, raw_text, nop_present);
    measure(row++, "A 2 chang", 2, tui_console_fill, tui_console_text, nop_present);
    measure(row++, "B 2 chang", 2, pend_fill, pend_text, present_two_tables);
    measure(row++, "C 2 chang", 2, pend_fill, pend_text, present_read_ram);
    measure(row++, "D 2 chang", 2, raw_fill, raw_text, nop_present);
    measure(row++, "A 3 pingp", 3, tui_console_fill, tui_console_text, nop_present);
    measure(row++, "B 3 pingp", 3, pend_fill, pend_text, present_two_tables);
    measure(row++, "C 3 pingp", 3, pend_fill, pend_text, present_read_ram);
    measure(row++, "D 3 pingp", 3, raw_fill, raw_text, nop_present);

    put_text(0, 8, "cycles instr stallmem stallmmio, per cell x10");

    return 0;
}
