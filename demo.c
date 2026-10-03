#include <stdio.h>

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
#define CMD_MOUSE_TOGGLE  110
#define CMD_LANG_BASIC    111
#define CMD_LANG_FORTH    112
#define CMD_LANG_C        113
#define CMD_OPTIMIZATION  114
#define CMD_DEMO_CONTROLS 115
#define CMD_DEMO_LAYOUT   116
#define CMD_SCROLL_V      117
#define CMD_SCROLL_H      118
#define CMD_DEMO_EDITOR   119
#define CMD_EDITOR_STATE  120

#define DEMO_NONE         0
#define DEMO_CONTROLS     1
#define DEMO_LAYOUT       2
#define DEMO_EDITOR       3

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

static const char *scroll_items[] = {
    "Item 01", "Item 02", "Item 03", "Item 04", "Item 05",
    "Item 06", "Item 07", "Item 08", "Item 09", "Item 10",
    "Item 11", "Item 12", "Item 13", "Item 14", "Item 15",
    "Item 16", "Item 17", "Item 18", "Item 19", "Item 20"
};

static const char *short_items[] = {
    "One",
    "Two",
    "Three"
};

static const char *optimization_items[] = {
    "None",
    "Size",
    "Speed",
    "Debug"
};

static const char *editor_sample[] = {
    "10 REM Editor demo: this is only sample text",
    "20 PRINT \"HELLO FROM THE EDITOR\"",
    "30 FOR I = 1 TO 10",
    "40     PRINT \"I = \"; I; \" and a rather long line that needs the horizontal scroll bar to be read completely\"",
    "50 NEXT I",
    "60 ",
    "70 REM Cursor keys, Home, End, Page Up and Page Down move around",
    "80 REM Type, Enter, Backspace and Delete edit the text",
    "90 REM The mouse places the cursor and drives the scroll bars",
    "100 LET A = 1",
    "110 LET B = 2",
    "120 LET C = A + B",
    "130 PRINT C",
    "140 IF C > 2 THEN PRINT \"C IS GREATER THAN TWO\"",
    "150 GOTO 170",
    "160 PRINT \"NEVER PRINTED\"",
    "170 PRINT \"BYE\"",
    "180 END"
};

#define EDITOR_SAMPLE_LINES \
    ((int)(sizeof(editor_sample) / sizeof(editor_sample[0])))

static const char *layout_file_items[] = {
    "src/",
    "  main.c",
    "include/",
    "  tui.h",
    "Makefile"
};

typedef struct App {
    int running;
    int active_demo;

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
    TuiWindow note_window;
    TuiWindow code_window;

    TuiLabel label;
    TuiButton button;
    TuiCheckBox mouse_checkbox;
    TuiRadioButton basic_radio;
    TuiRadioButton forth_radio;
    TuiRadioButton c_radio;
    TuiComboBox optimization_combo;
    TuiComboBox scroll_combo;
    char edit_buffer[64];
    TuiEdit edit;
    TuiListBox listbox;
    TuiListBox scroll_list;
    TuiListBox short_list;
    char note_buffer[256];
    TuiTextArea note_area;
    char code_buffer[1024];
    TuiTextArea code_area;
    char help_buffer[256];
    TuiTextArea help_area;
    TuiScrollBar vscroll;
    TuiScrollBar hscroll;
    TuiLabel vscroll_label;
    TuiLabel hscroll_label;
    char vscroll_text[24];
    char hscroll_text[24];

    TuiWindow editor_view;
    char editor_buffer[8192];
    TuiLinearTextModel editor_model;
    TuiEditor editor;
    char editor_status[48];

    TuiWindow layout_view;
    TuiWindow project_window;
    TuiWindow editor_window;
    TuiWindow inspector_window;
    TuiWindow output_window;
    TuiListBox layout_files;
    TuiLabel layout_project_note;
    TuiLabel layout_code_line_1;
    TuiLabel layout_code_line_2;
    TuiLabel layout_code_line_3;
    TuiLabel layout_code_line_4;
    TuiLabel layout_code_line_5;
    TuiLabel layout_inspector_title;
    TuiLabel layout_inspector_file;
    TuiLabel layout_inspector_type;
    TuiLabel layout_inspector_size;
    TuiLabel layout_output_line_1;
    TuiLabel layout_output_line_2;
} App;

typedef struct CommandEntry {
    int command;
    const char *description;
} CommandEntry;

static void demo_show_controls(App *app);
static void demo_show_layout(App *app);
static void demo_show_editor(App *app);

static CommandEntry command_table[] = {
    { CMD_NEW,           "Command: File -> New" },
    { CMD_OPEN,          "Command: File -> Open" },
    { CMD_SAVE,          "Command: File -> Save" },
    { CMD_RUN,           "Command: Run -> Run" },
    { CMD_STOP,          "Command: Run -> Stop" },
    { CMD_ABOUT,         "Command: Help -> About" },
    { CMD_LIST_OPEN,     "Command: ListBox activate" },
    { CMD_BUTTON_ACTION, "Command: Button click" },
    { CMD_MOUSE_TOGGLE,  "Command: Mouse support changed" },
    { CMD_LANG_BASIC,    "Command: Language -> BASIC" },
    { CMD_LANG_FORTH,    "Command: Language -> Forth" },
    { CMD_LANG_C,        "Command: Language -> C" },
    { CMD_OPTIMIZATION,  "Command: Optimization changed" },
    { CMD_SCROLL_V,      "Command: Vertical scroll bar" },
    { CMD_SCROLL_H,      "Command: Horizontal scroll bar" },
    { CMD_DEMO_CONTROLS, "Demo: Controls" },
    { CMD_DEMO_LAYOUT,   "Demo: Layout / Mini IDE" },
    { CMD_DEMO_EDITOR,   "Demo: Editor" },
    { CMD_QUIT,          "Command: File -> Exit" }
};

#define COMMAND_COUNT \
    ((int)(sizeof(command_table) / sizeof(command_table[0])))

static void demo_update_scroll_labels(App *app)
{
    sprintf(app->vscroll_text, "Vertical: %d",
            tui_scrollbar_get_value(&app->vscroll));
    sprintf(app->hscroll_text, "Horizontal: %d",
            tui_scrollbar_get_value(&app->hscroll));
    tui_label_set_text(&app->vscroll_label, app->vscroll_text);
    tui_label_set_text(&app->hscroll_label, app->hscroll_text);
}

/* The editor state is queried from the editor, never from its buffer. */
static void demo_update_editor_status(App *app)
{
    TuiEditorPosition position;

    tui_editor_get_position(&app->editor, &position);
    sprintf(app->editor_status, "Ln %d, Col %d%s",
            position.line + 1,
            position.column + 1,
            tui_editor_is_modified(&app->editor) ?
            "    Modified" : "");
    tui_statusbar_set_text(&app->status_bar, app->editor_status);
}

static int dispatch_command(App *app, int command)
{
    int i;

    if (command == CMD_EDITOR_STATE) {
        demo_update_editor_status(app);
        return 1;
    }

    for (i = 0; i < COMMAND_COUNT; ++i) {
        if (command_table[i].command == command) {
            tui_statusbar_set_text(&app->status_bar,
                                   command_table[i].description);

            if (command == CMD_QUIT)
                app->running = 0;
            else if (command == CMD_SCROLL_V ||
                     command == CMD_SCROLL_H)
                demo_update_scroll_labels(app);
            else if (command == CMD_DEMO_CONTROLS)
                demo_show_controls(app);
            else if (command == CMD_DEMO_LAYOUT)
                demo_show_layout(app);
            else if (command == CMD_DEMO_EDITOR)
                demo_show_editor(app);

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
    else if (control == &app->mouse_checkbox.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: CheckBox");
    else if (control == &app->basic_radio.control ||
             control == &app->forth_radio.control ||
             control == &app->c_radio.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: RadioButton");
    else if (control == &app->optimization_combo.control ||
             control == &app->scroll_combo.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: ComboBox");
    else if (control == &app->listbox.control ||
             control == &app->scroll_list.control ||
             control == &app->short_list.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: ListBox");
    else if (control == &app->note_area.control ||
             control == &app->code_area.control ||
             control == &app->help_area.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: TextArea");
    else if (control == &app->button.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: Button");
    else if (control == &app->vscroll.control ||
             control == &app->hscroll.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: ScrollBar");
    else if (control == &app->label.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Control: Label");
    else if (control == &app->editor.control)
        return;
    else if (control == &app->layout_files.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Layout: Project files");
    else if (control == &app->label_window.control ||
             control == &app->button_window.control ||
             control == &app->edit_window.control ||
             control == &app->list_window.control ||
             control == &app->note_window.control ||
             control == &app->code_window.control ||
             control == &app->editor_view.control)
        tui_statusbar_set_text(&app->status_bar,
                               "Window: drag title to move");
}

static void demo_build_controls(App *app)
{
    tui_window_init(&app->left, 0, 0, 12, 5, "Left");
    app->left.control.dock = TUI_DOCK_LEFT;
    app->left.control.attr = TUI_ATTR(TUI_WHITE, TUI_RED);

    tui_window_init(&app->right, 0, 0, 15, 5, "Right");
    app->right.control.dock = TUI_DOCK_RIGHT;
    app->right.control.attr = TUI_ATTR(TUI_BLACK, TUI_YELLOW);

    tui_window_init(&app->workspace, 0, 0, 1, 1, "Workspace");
    app->workspace.control.dock = TUI_DOCK_FILL;

    tui_window_init(&app->label_window, 0, 0, 25, 8, "Label");
    app->label_window.control.attr = TUI_ATTR(TUI_WHITE, TUI_BLUE);
    tui_window_init(&app->button_window, 26, 0, 25, 8, "Button");
    tui_window_set_flags(&app->button_window,
                         TUI_WINDOW_TITLE_LEFT);
    app->button_window.control.attr = TUI_ATTR(TUI_BLACK, TUI_CYAN);
    tui_window_init(&app->edit_window, 0, 8, 25, 8, "Edit");
    app->edit_window.control.attr = TUI_ATTR(TUI_BLACK, TUI_YELLOW);
    tui_window_set_flags(&app->edit_window,
                         TUI_WINDOW_ACTIVE_DOUBLE |
                         TUI_WINDOW_TITLE_CENTER);
    tui_window_init(&app->list_window, 26, 8, 25, 8, "ListBox");
    app->list_window.control.attr = TUI_ATTR(TUI_WHITE, TUI_RED);
    tui_window_set_flags(&app->list_window,
                         TUI_WINDOW_FIXED |
                         TUI_WINDOW_ACTIVE_DOUBLE |
                         TUI_WINDOW_TITLE_RIGHT);

    tui_window_init(&app->note_window, 0, 16, 25, 5, "TextArea");
    app->note_window.control.attr = TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);
    tui_window_init(&app->code_window, 26, 16, 25, 5, "Scrolling");
    app->code_window.control.attr = TUI_ATTR(TUI_WHITE, TUI_BLUE);

    tui_label_init(&app->label, 1, 2, "A simple text label");

    tui_button_init(&app->button, 4, 5, 16,
                    "Press me", CMD_BUTTON_ACTION);

    tui_checkbox_init(&app->mouse_checkbox, 1, 4, 21,
                      "Mouse support");
    tui_checkbox_set_command(&app->mouse_checkbox,
                             CMD_MOUSE_TOGGLE);

    tui_radiobutton_init(&app->basic_radio, 1, 1, 21,
                         "BASIC", 1);
    tui_radiobutton_set_command(&app->basic_radio, CMD_LANG_BASIC);
    tui_radiobutton_init(&app->forth_radio, 1, 2, 21,
                         "Forth", 1);
    tui_radiobutton_set_command(&app->forth_radio, CMD_LANG_FORTH);
    tui_radiobutton_init(&app->c_radio, 1, 3, 21,
                         "C", 1);
    tui_radiobutton_set_command(&app->c_radio, CMD_LANG_C);
    tui_radiobutton_set_checked(&app->forth_radio, 1);

    app->edit_buffer[0] = '\0';
    tui_edit_init(&app->edit, 1, 2, 21,
                  app->edit_buffer,
                  (int)sizeof(app->edit_buffer));
    tui_edit_set_text(&app->edit, "Type here");

    tui_combobox_init(&app->optimization_combo, 1, 4, 21,
                      optimization_items, 4);
    tui_combobox_set_command(&app->optimization_combo,
                             CMD_OPTIMIZATION);

    tui_combobox_init(&app->scroll_combo, 1, 5, 21,
                      scroll_items, 20);
    tui_combobox_set_scrollbar(&app->scroll_combo, 1);

    app->note_buffer[0] = '\0';
    tui_textarea_init(&app->note_area, 0, 0, 23, 3,
                      app->note_buffer,
                      (int)sizeof(app->note_buffer));
    tui_textarea_set_text(&app->note_area,
                          "Hello world\n"
                          "This is a multiline\n"
                          "editable text area.");

    app->code_buffer[0] = '\0';
    tui_textarea_init(&app->code_area, 0, 0, 23, 3,
                      app->code_buffer,
                      (int)sizeof(app->code_buffer));
    tui_textarea_set_text(&app->code_area,
        "10 PRINT \"A long line that needs horizontal scrolling\"\n"
        "20 FOR I = 1 TO 10\n"
        "30   PRINT I\n"
        "40 NEXT I\n"
        "50 REM The vertical bar\n"
        "60 REM appears when there\n"
        "70 REM are more lines\n"
        "80 END");

    app->help_buffer[0] = '\0';
    tui_textarea_init(&app->help_area, 0, 17, 13, 4,
                      app->help_buffer,
                      (int)sizeof(app->help_buffer));
    tui_textarea_set_text(&app->help_area,
                          "Read-only text.\nArrows and the\n"
                          "mouse scroll it.\nNothing can be\n"
                          "edited here.");
    tui_textarea_set_readonly(&app->help_area, 1);

    tui_listbox_init(&app->listbox, 1, 1, 21, 5,
                     demo_items, 10);
    tui_listbox_set_command(&app->listbox, CMD_LIST_OPEN);

    tui_listbox_init(&app->scroll_list, 0, 0, 10, 6,
                     scroll_items, 20);
    tui_listbox_set_scrollbar(&app->scroll_list, 1);
    tui_listbox_init(&app->short_list, 0, 7, 10, 3,
                     short_items, 3);
    tui_listbox_set_scrollbar(&app->short_list, 1);

    tui_scrollbar_init(&app->vscroll, 1, 1, 12,
                       TUI_VERTICAL, CMD_SCROLL_V);
    tui_scrollbar_set_range(&app->vscroll, 0, 100);
    tui_scrollbar_set_page(&app->vscroll, 20);
    tui_scrollbar_init(&app->hscroll, 1, 14, 12,
                       TUI_HORIZONTAL, CMD_SCROLL_H);
    tui_scrollbar_set_range(&app->hscroll, 0, 50);
    tui_scrollbar_set_page(&app->hscroll, 10);
    tui_label_init(&app->vscroll_label, 3, 1, "");
    tui_label_init(&app->hscroll_label, 1, 16, "");
    demo_update_scroll_labels(app);


    tui_add(&app->workspace.control, &app->label_window.control);
    tui_add(&app->workspace.control, &app->button_window.control);
    tui_add(&app->workspace.control, &app->edit_window.control);
    tui_add(&app->workspace.control, &app->list_window.control);
    tui_add(&app->workspace.control, &app->note_window.control);
    tui_add(&app->workspace.control, &app->code_window.control);

    tui_add(&app->label_window.control, &app->label.control);
    tui_add(&app->label_window.control, &app->mouse_checkbox.control);
    tui_add(&app->button_window.control, &app->button.control);
    tui_add(&app->button_window.control, &app->basic_radio.control);
    tui_add(&app->button_window.control, &app->forth_radio.control);
    tui_add(&app->button_window.control, &app->c_radio.control);
    tui_add(&app->edit_window.control, &app->edit.control);
    tui_add(&app->edit_window.control,
            &app->optimization_combo.control);
    tui_add(&app->edit_window.control, &app->scroll_combo.control);
    tui_add(&app->list_window.control, &app->listbox.control);
    tui_add(&app->note_window.control, &app->note_area.control);
    tui_add(&app->code_window.control, &app->code_area.control);

    tui_add(&app->left.control, &app->scroll_list.control);
    tui_add(&app->left.control, &app->short_list.control);

    tui_add(&app->right.control, &app->vscroll.control);
    tui_add(&app->right.control, &app->vscroll_label.control);
    tui_add(&app->right.control, &app->hscroll.control);
    tui_add(&app->right.control, &app->hscroll_label.control);
    tui_add(&app->right.control, &app->help_area.control);
}

static void demo_build_editor(App *app)
{
    int i;

    tui_window_init(&app->editor_view, 0, 0, 1, 1, "Editor Demo");
    app->editor_view.control.dock = TUI_DOCK_FILL;

    app->editor_buffer[0] = '\0';
    tui_linear_text_model_init(&app->editor_model,
                               app->editor_buffer,
                               (int)sizeof(app->editor_buffer));

    for (i = 0; i < EDITOR_SAMPLE_LINES; ++i) {
        const char *line;
        int length;

        line = editor_sample[i];

        for (length = 0; line[length] != '\0'; ++length)
            ;

        tui_text_model_insert(&app->editor_model.model,
                              tui_text_model_length(
                                  &app->editor_model.model),
                              line, length);
        tui_text_model_insert(&app->editor_model.model,
                              tui_text_model_length(
                                  &app->editor_model.model),
                              "\n", 1);
    }

    /* The editor only sees the generic model, not the buffer. */
    tui_editor_init(&app->editor, 0, 0, 1, 1,
                    &app->editor_model.model);
    app->editor.control.dock = TUI_DOCK_FILL;
    app->editor.control.attr = TUI_ATTR(TUI_LIGHTGRAY, TUI_BLUE);
    tui_editor_set_command(&app->editor, CMD_EDITOR_STATE);

    tui_add(&app->editor_view.control, &app->editor.control);
}

static void demo_build_layout(App *app)
{
    tui_window_init(&app->layout_view, 0, 0, 1, 1,
                    "Layout Demo / Mini IDE");
    app->layout_view.control.dock = TUI_DOCK_FILL;

    tui_window_init(&app->project_window, 0, 0, 22, 5,
                    "Project");
    app->project_window.control.dock = TUI_DOCK_LEFT;
    app->project_window.control.attr =
        TUI_ATTR(TUI_WHITE, TUI_BLUE);

    tui_window_init(&app->inspector_window, 0, 0, 24, 5,
                    "Inspector");
    app->inspector_window.control.dock = TUI_DOCK_RIGHT;
    app->inspector_window.control.attr =
        TUI_ATTR(TUI_BLACK, TUI_CYAN);

    tui_window_init(&app->output_window, 0, 0, 1, 5,
                    "Output");
    app->output_window.control.dock = TUI_DOCK_BOTTOM;
    app->output_window.control.attr =
        TUI_ATTR(TUI_BLACK, TUI_LIGHTGRAY);

    tui_window_init(&app->editor_window, 0, 0, 1, 1,
                    "Editor - main.c");
    app->editor_window.control.dock = TUI_DOCK_FILL;
    app->editor_window.control.attr =
        TUI_ATTR(TUI_LIGHTGRAY, TUI_BLACK);

    tui_listbox_init(&app->layout_files, 1, 1, 18, 6,
                     layout_file_items, 5);
    tui_label_init(&app->layout_project_note, 1, 8,
                   "Enter: open file");

    tui_label_init(&app->layout_code_line_1, 1, 1,
                   "#include \"tui.h\"");
    tui_label_init(&app->layout_code_line_2, 1, 2,
                   "int main(void)");
    tui_label_init(&app->layout_code_line_3, 1, 3, "{");
    tui_label_init(&app->layout_code_line_4, 1, 4,
                   "    tui_init();");
    tui_label_init(&app->layout_code_line_5, 1, 5,
                   "    return 0;");

    tui_label_init(&app->layout_inspector_title, 1, 1,
                   "File: main.c");
    tui_label_init(&app->layout_inspector_file, 1, 3,
                   "Type: C source");
    tui_label_init(&app->layout_inspector_type, 1, 5,
                   "Dock: fill");
    tui_label_init(&app->layout_inspector_size, 1, 7,
                   "Layout: resizable");

    tui_label_init(&app->layout_output_line_1, 1, 1,
                   "Build succeeded.");
    tui_label_init(&app->layout_output_line_2, 1, 2,
                   "0 errors, 0 warnings");

    tui_add(&app->project_window.control,
            &app->layout_files.control);
    tui_add(&app->project_window.control,
            &app->layout_project_note.control);
    tui_add(&app->editor_window.control,
            &app->layout_code_line_1.control);
    tui_add(&app->editor_window.control,
            &app->layout_code_line_2.control);
    tui_add(&app->editor_window.control,
            &app->layout_code_line_3.control);
    tui_add(&app->editor_window.control,
            &app->layout_code_line_4.control);
    tui_add(&app->editor_window.control,
            &app->layout_code_line_5.control);
    tui_add(&app->inspector_window.control,
            &app->layout_inspector_title.control);
    tui_add(&app->inspector_window.control,
            &app->layout_inspector_file.control);
    tui_add(&app->inspector_window.control,
            &app->layout_inspector_type.control);
    tui_add(&app->inspector_window.control,
            &app->layout_inspector_size.control);
    tui_add(&app->output_window.control,
            &app->layout_output_line_1.control);
    tui_add(&app->output_window.control,
            &app->layout_output_line_2.control);

    tui_add(&app->layout_view.control,
            &app->project_window.control);
    tui_add(&app->layout_view.control,
            &app->inspector_window.control);
    tui_add(&app->layout_view.control,
            &app->output_window.control);
    tui_add(&app->layout_view.control,
            &app->editor_window.control);
}

static void demo_hide_active(App *app)
{
    TuiEvent event;

    if (app->active_demo == DEMO_CONTROLS &&
        (app->optimization_combo.open ||
         app->scroll_combo.open)) {
        event.type = TUI_EV_KEY;
        event.key = TUI_KEY_ESCAPE;
        event.command = TUI_CMD_NONE;
        event.source = 0;
        event.mouse_x = 0;
        event.mouse_y = 0;
        event.mouse_action = 0;
        event.mouse_buttons = 0;
        tui_dispatch(&app->desktop, &event);
    }

    tui_desktop_set_focus(&app->desktop, 0);
    tui_desktop_clear_capture(&app->desktop);

    if (app->active_demo == DEMO_CONTROLS) {
        app->label_window.dragging = 0;
        app->button_window.dragging = 0;
        app->edit_window.dragging = 0;
        app->list_window.dragging = 0;
        app->note_window.dragging = 0;
        app->code_window.dragging = 0;
        app->button.pressed = 0;
        tui_remove(&app->left.control);
        tui_remove(&app->right.control);
        tui_remove(&app->workspace.control);
    } else if (app->active_demo == DEMO_LAYOUT) {
        tui_remove(&app->layout_view.control);
    } else if (app->active_demo == DEMO_EDITOR) {
        app->editor_view.dragging = 0;
        tui_remove(&app->editor_view.control);
    }

    app->active_demo = DEMO_NONE;
}

static void demo_show_controls(App *app)
{
    if (app->active_demo != DEMO_CONTROLS) {
        demo_hide_active(app);
        tui_add(&app->desktop.control, &app->left.control);
        tui_add(&app->desktop.control, &app->right.control);
        tui_add(&app->desktop.control, &app->workspace.control);
        app->active_demo = DEMO_CONTROLS;
    }

    tui_desktop_set_focus(&app->desktop, &app->edit.control);
    tui_statusbar_set_text(&app->status_bar,
                           "Demo: Controls");
}

static void demo_show_layout(App *app)
{
    if (app->active_demo != DEMO_LAYOUT) {
        demo_hide_active(app);
        tui_add(&app->desktop.control,
                &app->layout_view.control);
        app->active_demo = DEMO_LAYOUT;
    }

    tui_desktop_set_focus(&app->desktop,
                          &app->layout_files.control);
    tui_statusbar_set_text(&app->status_bar,
                           "Demo: Layout / Mini IDE");
}

static void demo_show_editor(App *app)
{
    if (app->active_demo != DEMO_EDITOR) {
        demo_hide_active(app);
        tui_add(&app->desktop.control, &app->editor_view.control);
        app->active_demo = DEMO_EDITOR;
    }

    tui_desktop_set_focus(&app->desktop, &app->editor.control);
    demo_update_editor_status(app);
}

static TuiMenuItem file_items[] = {
    { "New",     CMD_NEW,  TUI_KEY_NONE, 0 },
    { "Open...", CMD_OPEN, TUI_KEY_F3,   0 },
    { "Save",    CMD_SAVE, TUI_KEY_F2,   0 },
    { 0,         0,        TUI_KEY_NONE, TUI_MENU_SEPARATOR },
    { "Exit",    CMD_QUIT, TUI_KEY_NONE, 0 }
};

static TuiMenuItem demo_items_menu[] = {
    { "Controls",          CMD_DEMO_CONTROLS, TUI_KEY_NONE, 0 },
    { "Layout / Mini IDE", CMD_DEMO_LAYOUT,   TUI_KEY_NONE, 0 },
    { "Editor",            CMD_DEMO_EDITOR,   TUI_KEY_NONE, 0 }
};

static TuiMenuItem run_items[] = {
    { "Run",     CMD_RUN,  TUI_KEY_F5,   0 },
    { "Stop",    CMD_STOP, TUI_KEY_NONE, TUI_MENU_DISABLED }
};

static TuiMenuItem help_items[] = {
    { "About",   CMD_ABOUT, TUI_KEY_NONE, 0 }
};

static TuiMenu menus[] = {
    { "File", file_items,      5 },
    { "Demo", demo_items_menu, 3 },
    { "Run",  run_items,       2 },
    { "Help", help_items,      1 }
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

    tui_menubar_init(&app.menu_bar, menus, 4);
    tui_statusbar_init(&app.status_bar, status_items, 4);
    tui_statusbar_set_text(&app.status_bar, "Ready");

    tui_add(&app.desktop.control, &app.menu_bar.control);
    tui_add(&app.desktop.control, &app.status_bar.control);

    app.active_demo = DEMO_NONE;
    demo_build_controls(&app);
    demo_build_layout(&app);
    demo_build_editor(&app);
    demo_show_controls(&app);

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
