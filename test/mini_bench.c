/*
 * Microbenchmark de la consola de la MiniCPU (make mini-bench MINI_BENCH_MODE=n):
 * cuenta las instrucciones que cuesta escribir celdas, en el simulador. El resultado
 * es la linea "HALT tras N instrucciones" que imprime cpusim; el coste por celda es
 * (N del modo - N del modo 8) / 3968, que son 62 pasadas de 64 celdas.
 *
 *   8   no escribe nada: el arranque de la consola y el relleno de las cadenas
 *   9   tui_console_cell sobre celdas que ya tienen eso (la copia de la pantalla)
 *   10  tui_console_cell sobre celdas que cambian en cada pasada
 *   4   tui_console_fill, sin cambio
 *   5   tui_console_fill, cambiando
 *   6   tui_console_text, sin cambio
 *   7   tui_console_text, cambiando
 *
 * Medido el 6/10/2026, instrucciones por celda: 9 = 47, 10 = 100, 4 = 11, 5 = 21,
 * 6 = 15, 7 = 27. Son instrucciones, no tiempo: en la placa pesan mas las esperas de
 * memoria de datos, y entrar y salir de una funcion guarda registros en la pila.
 */
#define TUI_BACKEND_MMIO
#include "../console_mini.c"

#ifndef BENCH_MODE
#define BENCH_MODE 8
#endif
#define PASSES 62

int main(void)
{
    static char sa[65];
    static char sb[65];
    int i;
    int j;

    tui_console_init();

    for (i = 0; i < 64; ++i) {
        sa[i] = 'a';
        sb[i] = 'b';
    }
    sa[64] = 0;
    sb[64] = 0;

    for (j = 0; j < PASSES; ++j) {
#if BENCH_MODE == 4
        tui_console_fill(0, 5, 64, 'a', 7);
#elif BENCH_MODE == 5
        tui_console_fill(0, 5, 64, 'a' + (j & 1), 7);
#elif BENCH_MODE == 6
        tui_console_text(0, 5, sa, 64, 7);
#elif BENCH_MODE == 7
        tui_console_text(0, 5, (j & 1) ? sb : sa, 64, 7);
#elif BENCH_MODE == 9
        for (i = 0; i < 64; ++i)
            tui_console_cell(i, 5, 'a', 7);
#elif BENCH_MODE == 10
        for (i = 0; i < 64; ++i)
            tui_console_cell(i, 5, 'a' + (j & 1), 7);
#endif
    }

    return 0;
}
