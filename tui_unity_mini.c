/*
 * MiniCPU unity build. The MMIO backend is selected before the common
 * translation unit, and demo.c omits its hosted stdio formatting in this mode.
 */
#define TUI_BACKEND_MMIO
#include "tui_unity.c"
