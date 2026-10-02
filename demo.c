#include "tui.h"

#define CMD_OK    100
#define CMD_QUIT  101

int main(void)
{
    TuiWindow window;
    TuiLabel label;
    TuiButton ok;
    TuiButton quit;
    TuiEvent event;
    int running;

    if (!tui_init())
        return 1;

    tui_window_init(&window, 10, 5, 50, 10, "Demo");

    tui_label_init(&label,
                   2, 2,
                   "Esto es una prueba de la TUI");

    tui_button_init(&ok,
                    8, 5, 14,
                    "Aceptar",
                    CMD_OK);

    tui_button_init(&quit,
                    26, 5, 12,
                    "Salir",
                    CMD_QUIT);

    tui_add(&window.control, &label.control);
    tui_add(&window.control, &ok.control);
    tui_add(&window.control, &quit.control);

    window.focused = &ok.control;

    running = 1;

    while (running) {
        tui_draw(&window.control);
        tui_read_event(&event);

        if (event.type == TUI_EV_KEY &&
            event.key == TUI_KEY_ESCAPE) {
            running = 0;
            continue;
        }

        tui_dispatch(&window, &event);

        if (event.type == TUI_EV_COMMAND) {
            if (event.command == CMD_OK) {
                tui_label_set_text(
                    &label,
                    "Has pulsado Aceptar.");
            } else if (event.command == CMD_QUIT) {
                running = 0;
            }
        }
    }

    tui_shutdown();
    return 0;
}
