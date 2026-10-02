#include "tui.h"

#define CMD_OK     100
#define CMD_QUIT   101
#define CMD_NEW    102
#define CMD_OPEN   103
#define CMD_SAVE   104
#define CMD_RUN    105
#define CMD_STOP   106
#define CMD_ABOUT  107


static TuiMenuItem file_items[] = {
    { "New",     CMD_NEW,  TUI_KEY_NONE, 0 },
    { "Open...", CMD_OPEN, TUI_KEY_F3,   0 },
    { "Save",    CMD_SAVE, TUI_KEY_F2,   0 },
    { 0,         0,        TUI_KEY_NONE, TUI_MENU_SEPARATOR },
    { "Exit",    CMD_QUIT, TUI_KEY_NONE, 0 }
};

static TuiMenuItem run_items[] = {
    { "Run",     CMD_RUN,  TUI_KEY_F5,   0 },
    { "Stop",    CMD_STOP, TUI_KEY_NONE, TUI_MENU_DISABLED }
};

static TuiMenuItem help_items[] = {
    { "About",   CMD_ABOUT, TUI_KEY_NONE, 0 }
};

static TuiMenu menus[] = {
    { "File", file_items, 5 },
    { "Run",  run_items,  2 },
    { "Help", help_items, 1 }
};

static TuiStatusItem status_items[] = {
    { "Help", TUI_KEY_F1, CMD_ABOUT },
    { "Save", TUI_KEY_F2, CMD_SAVE  },
    { "Open", TUI_KEY_F3, CMD_OPEN  },
    { "Run",  TUI_KEY_F5, CMD_RUN   }
};

int main(void)
{
    TuiDesktop desktop;
    TuiWindow window;
    TuiLabel label;
    TuiButton ok;
    TuiButton quit;
    TuiMenuBar menu_bar;
    TuiStatusBar status_bar;
    TuiEvent event;
    int running;

    if (!tui_init())
        return 1;

    /*
     * Desktop
     */
    tui_desktop_init(&desktop);

    /*
     * Menu bar
     */
    tui_menubar_init(&menu_bar, menus, 3);

    tui_statusbar_init(
        &status_bar,
        status_items,
        4);

    tui_statusbar_set_text(
        &status_bar,
        "Ready");

    /*
     * Window
     */
    tui_window_init(
        &window,
        10, 5,
        50, 10,
        "Demo");

    /*
     * Controls
     */
    tui_label_init(
        &label,
        2, 2,
        "Esto es una prueba de la TUI");

    tui_button_init(
        &ok,
        8, 5,
        14,
        "Aceptar",
        CMD_OK);

    tui_button_init(
        &quit,
        26, 5,
        12,
        "Salir",
        CMD_QUIT);

    /*
     * Build control tree.
     *
     * Desktop
     *   |
     *   +-- Window
     *        |
     *        +-- Label
     *        +-- OK
     *        +-- Quit
     */    
    tui_add(&desktop.control, &window.control);
    tui_add(&desktop.control, &menu_bar.control);
    tui_add(&desktop.control, &status_bar.control);
    
    tui_add(&window.control, &label.control);
    tui_add(&window.control, &ok.control);
    tui_add(&window.control, &quit.control);

    /*
     * Initial focus.
     */
    tui_desktop_set_focus(
        &desktop,
        &ok.control);

    /*
     * Main loop.
     */
    running = 1;

    while (running) {

        tui_draw(&desktop);

        tui_read_event(&event);

        if (event.type == TUI_EV_KEY &&
            event.key == TUI_KEY_ESCAPE &&
            desktop.capture == 0) {

                running = 0;

        } else {
            tui_dispatch(&desktop, &event);
        }

        if (event.type == TUI_EV_COMMAND) {

            if (event.command == CMD_OK) {

                tui_label_set_text(
                    &label,
                    "Has pulsado Aceptar.");

            } else if (event.command == CMD_NEW) {

                tui_label_set_text(
                    &label,
                    "File -> New");

            } else if (event.command == CMD_OPEN) {

                tui_label_set_text(
                    &label,
                    "File -> Open");

            } else if (event.command == CMD_SAVE) {

                tui_label_set_text(
                    &label,
                    "File -> Save");

            } else if (event.command == CMD_RUN) {

                tui_label_set_text(
                    &label,
                    "Run -> Run");

            } else if (event.command == CMD_ABOUT) {

                tui_label_set_text(
                    &label,
                    "Help -> About");

            } else if (event.command == CMD_QUIT) {

                running = 0;
            }
        }
    }

    tui_shutdown();

    return 0;
}
