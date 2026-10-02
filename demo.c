#include "tui.h"

#define CMD_QUIT          101
#define CMD_NEW           102
#define CMD_OPEN          103
#define CMD_SAVE          104
#define CMD_RUN           105
#define CMD_STOP          106
#define CMD_ABOUT         107
#define CMD_LIST_OPEN     108
#define CMD_BUTTON_ACTION 109

static const char *demo_items[] = {
    "Apple",
    "Banana",
    "Orange",
    "Peach",
    "Pear",
    "Strawberry",
    "Watermelon",
    "Cherry",
    "Lemon",
    "Mango"
};

typedef struct App {
    int running;

    TuiDesktop desktop;
    TuiMenuBar menu_bar;
    TuiStatusBar status_bar;

    TuiWindow left;
    TuiWindow right;
    TuiWindow workspace;
    TuiWindow label_window;
    TuiWindow button_window;
    TuiWindow edit_window;
    TuiWindow list_window;

    TuiLabel label;
    TuiButton button;
    char edit_buffer[64];
    TuiEdit edit;
    TuiListBox listbox;
} App;

typedef struct CommandEntry {
    int command;
    const char *description;
} CommandEntry;

static CommandEntry command_table[] = {
    { CMD_NEW,           "Command: File -> New" },
    { CMD_OPEN,          "Command: File -> Open" },
    { CMD_SAVE,          "Command: File -> Save" },
    { CMD_RUN,           "Command: Run -> Run" },
    { CMD_STOP,          "Command: Run -> Stop" },
    { CMD_ABOUT,         "Command: Help -> About" },
    { CMD_LIST_OPEN,     "Command: ListBox activate" },
    { CMD_BUTTON_ACTION, "Command: Button click" },
    { CMD_QUIT,          "Command: File -> Exit" }
};

#define COMMAND_COUNT \
    ((int)(sizeof(command_table) / sizeof(command_table[0])))

static int dispatch_command(App *app, int command)
{
    int i;

    for (i = 0; i < COMMAND_COUNT; ++i) {
        if (command_table[i].command == command) {
            tui_statusbar_set_text(&app->status_bar,
                                   command_table[i].description);

            if (command == CMD_QUIT)
                app->running = 0;

            return 1;
        }
    }

    return 0;
}

static void note_control_event(App *app, TuiEvent *event)
{
    TuiControl *control;

    control = 0;

    if (event->type == TUI_EV_KEY) {
        control = tui_desktop_get_focus(&app->desktop);
    } else if (event->type == TUI_EV_MOUSE &&
               event->mouse_action == TUI_MOUSE_DOWN) {
        control = tui_hit_test(&app->desktop.control,
                               event->mouse_x,
                               event->mouse_y);
    }

    if (control == &app->edit.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: Edit");
    else if (control == &app->listbox.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: ListBox");
    else if (control == &app->button.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: Button");
    else if (control == &app->label.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: Label");
    else if (control == &app->label_window.control ||
             control == &app->button_window.control ||
             control == &app->edit_window.control ||
             control == &app->list_window.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Window: drag title to move");
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

    tui_desktop_init(&app.desktop);

    tui_menubar_init(&app.menu_bar, menus, 3);
    tui_statusbar_init(&app.status_bar, status_items, 4);
    tui_statusbar_set_text(&app.status_bar, "Ready");

    tui_window_init(&app.left, 0, 0, 12, 5, "Left");
    app.left.control.dock = TUI_DOCK_LEFT;
    app.left.control.attr = TUI_ATTR(TUI_WHITE, TUI_RED);

    tui_window_init(&app.right, 0, 0, 15, 5, "Right");
    app.right.control.dock = TUI_DOCK_RIGHT;
    app.right.control.attr = TUI_ATTR(TUI_BLACK, TUI_YELLOW);

    tui_window_init(&app.workspace, 0, 0, 1, 1, "Workspace");
    app.workspace.control.dock = TUI_DOCK_FILL;

    tui_window_init(&app.label_window, 0, 0, 25, 8, "Label");
    app.label_window.control.attr = TUI_ATTR(TUI_WHITE, TUI_BLUE);
    tui_window_init(&app.button_window, 26, 0, 25, 8, "Button");
    app.button_window.control.attr = TUI_ATTR(TUI_BLACK, TUI_CYAN);
    tui_window_init(&app.edit_window, 0, 9, 25, 8, "Edit");
    app.edit_window.control.attr = TUI_ATTR(TUI_BLACK, TUI_YELLOW);
    tui_window_init(&app.list_window, 26, 9, 25, 8, "ListBox");
    app.list_window.control.attr = TUI_ATTR(TUI_WHITE, TUI_RED);

    tui_label_init(&app.label, 1, 2, "A simple text label");

    tui_button_init(&app.button, 4, 2, 16,
                    "Press me", CMD_BUTTON_ACTION);

    app.edit_buffer[0] = '\0';
    tui_edit_init(&app.edit, 1, 2, 21,
                  app.edit_buffer,
                  (int)sizeof(app.edit_buffer));
    tui_edit_set_text(&app.edit, "Type here");

    tui_listbox_init(&app.listbox, 1, 1, 21, 5,
                     demo_items, 10);
    tui_listbox_set_command(&app.listbox, CMD_LIST_OPEN);

    tui_add(&app.desktop.control, &app.menu_bar.control);
    tui_add(&app.desktop.control, &app.status_bar.control);
    tui_add(&app.desktop.control, &app.left.control);
    tui_add(&app.desktop.control, &app.right.control);
    tui_add(&app.desktop.control, &app.workspace.control);

    tui_add(&app.workspace.control, &app.label_window.control);
    tui_add(&app.workspace.control, &app.button_window.control);
    tui_add(&app.workspace.control, &app.edit_window.control);
    tui_add(&app.workspace.control, &app.list_window.control);

    tui_add(&app.label_window.control, &app.label.control);
    tui_add(&app.button_window.control, &app.button.control);
    tui_add(&app.edit_window.control, &app.edit.control);
    tui_add(&app.list_window.control, &app.listbox.control);

    tui_desktop_set_focus(&app.desktop, &app.edit.control);

    app.running = 1;

    while (app.running) {
        tui_draw(&app.desktop);
        tui_read_event(&event);

        if (event.type == TUI_EV_KEY &&
            event.key == TUI_KEY_ESCAPE &&
            app.desktop.capture == 0) {
            app.running = 0;
        } else {
            note_control_event(&app, &event);
            tui_dispatch(&app.desktop, &event);
        }

        if (event.type == TUI_EV_COMMAND)
            dispatch_command(&app, event.command);
    }

    tui_shutdown();

    return 0;
}
