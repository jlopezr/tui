/*
 * Unity build for compilers which cannot link separate object files.
 * Keep this include order aligned with the normal source list.
 */
#include "tui.c"
#include "tui_window.c"
#include "tui_button.c"
#include "tui_label.c"
#include "tui_edit.c"
#include "tui_listbox.c"
#include "tui_menu.c"
#include "tui_statusbar.c"
#include "tui_checkbox.c"
#include "tui_radiobutton.c"
#include "tui_combobox.c"
#include "tui_scrollbar.c"
#include "tui_textarea.c"

#if defined(TUI_BACKEND_NCURSES)
    #include "console_ncurses.c"
#elif defined(TUI_BACKEND_MMIO)
    #include "console_mmio.c"
#else
    #error No TUI backend selected
#endif

#include "demo.c"
