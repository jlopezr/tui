#include "tui.h"

#define CMD_OK     100
#define CMD_QUIT   101
#define CMD_NEW    102
#define CMD_OPEN   103
#define CMD_SAVE   104
#define CMD_RUN    105
#define CMD_STOP   106
#define CMD_ABOUT  107

typedef struct App {
    int running;

    TuiDesktop desktop;
    TuiMenuBar menu_bar;
    TuiStatusBar status_bar;

    TuiWindow window;
    TuiLabel label;
    TuiButton ok;
    TuiButton quit;
} App;

typedef void (*CommandFn)(App *app);

typedef struct CommandEntry {
    int command;
    CommandFn function;
} CommandEntry;

static void cmd_ok(App *app)
{
    tui_label_set_text(
        &app->label,
        "Has pulsado Aceptar.");
}

static void cmd_new(App *app)
{
    tui_label_set_text(
        &app->label,
        "File -> New");
}

static void cmd_open(App *app)
{
    tui_label_set_text(
        &app->label,
        "File -> Open");
}

static void cmd_save(App *app)
{
    tui_label_set_text(
        &app->label,
        "File -> Save");
}

static void cmd_run(App *app)
{
    tui_label_set_text(
        &app->label,
        "Run -> Run");
}

static void cmd_stop(App *app)
{
    tui_label_set_text(
        &app->label,
        "Run -> Stop");
}

static void cmd_about(App *app)
{
    tui_label_set_text(
        &app->label,
        "Help -> About");
}

static void cmd_quit(App *app)
{
    app->running = 0;
}

static CommandEntry command_table[] = {
    { CMD_OK,    cmd_ok    },
    { CMD_NEW,   cmd_new   },
    { CMD_OPEN,  cmd_open  },
    { CMD_SAVE,  cmd_save  },
    { CMD_RUN,   cmd_run   },
    { CMD_STOP,  cmd_stop  },
    { CMD_ABOUT, cmd_about },
    { CMD_QUIT,  cmd_quit  }
};

#define COMMAND_COUNT \
    ((int)(sizeof(command_table) / sizeof(command_table[0])))

static int dispatch_command(App *app, int command)
{
    int i;

    for (i = 0; i < COMMAND_COUNT; ++i) {
        if (command_table[i].command == command) {
            command_table[i].function(app);
            return 1;
        }
    }

    return 0;
}

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
    App app;
    TuiEvent event;

    if (!tui_init())
        return 1;

    /*
     * Desktop
     */
    tui_desktop_init(&app.desktop);

    /*
     * Menu bar
     */
    tui_menubar_init(&app.menu_bar, menus, 3);

    tui_statusbar_init(
        &app.status_bar,
        status_items,
        4);

    tui_statusbar_set_text(
        &app.status_bar,
        "Ready");

    /*
     * Window
     */
    tui_window_init(
        &app.window,
        10, 5,
        50, 10,
        "Demo");

    /*
     * Controls
     */
    tui_label_init(
        &app.label,
        2, 2,
        "Esto es una prueba de la TUI");

    tui_button_init(
        &app.ok,
        8, 5,
        14,
        "Aceptar",
        CMD_OK);

    tui_button_init(
        &app.quit,
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
    tui_add(&app.desktop.control, &app.window.control);
    tui_add(&app.desktop.control, &app.menu_bar.control);
    tui_add(&app.desktop.control, &app.status_bar.control);
    
    tui_add(&app.window.control, &app.label.control);
    tui_add(&app.window.control, &app.ok.control);
    tui_add(&app.window.control, &app.quit.control);

    /*
     * Initial focus.
     */
    tui_desktop_set_focus(
        &app.desktop,
        &app.ok.control);

    /*
     * Main loop.
     */
    app.running = 1;

    while (app.running) {

        tui_draw(&app.desktop);

        tui_read_event(&event);

        if (event.type == TUI_EV_KEY &&
            event.key == TUI_KEY_ESCAPE &&
            app.desktop.capture == 0) {

                app.running = 0;

        } else {
            tui_dispatch(&app.desktop, &event);
        }

        if (event.type == TUI_EV_COMMAND) {            
            dispatch_command(&app, event.command);
        }
    }

    tui_shutdown();

    return 0;
}
