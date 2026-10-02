/*
 * Single translation unit.
 *
 * This is the only .c file passed to the compiler.
 */

#include "tui.c"
#if defined(TUI_BACKEND_NCURSES)
    #include "console_ncurses.c"
#elif defined(TUI_BACKEND_MMIO)
    #include "console_mmio.c"
#else
    #error No TUI backend selected
#endif

#include "demo.c"
