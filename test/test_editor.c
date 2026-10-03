#include <string.h>

#include "test_support.h"

#define CMD_ED 77

typedef struct Fixture {
    TuiDesktop desktop;
    TuiLinearTextModel linear;
    TuiEditor editor;
    char buffer[256];
} Fixture;

static void ed_setup(Fixture *f, int capacity, int width, int height,
                     const char *text)
{
    test_init_desktop(&f->desktop);
    tui_linear_text_model_init(&f->linear, f->buffer, capacity);
    tui_linear_text_model_set_text(&f->linear, text);
    tui_editor_init(&f->editor, 2, 2, width, height, &f->linear.model);
    tui_editor_set_command(&f->editor, CMD_ED);
    tui_add(&f->desktop.control, &f->editor.control);
    tui_desktop_set_focus(&f->desktop, &f->editor.control);
}

/* Compares through the model interface, not through the buffer. */
static int same(TuiEditor *editor, const char *expected)
{
    char out[300];
    int n;

    n = tui_text_model_read(editor->model, 0, out, 299);
    out[n] = '\0';

    return strcmp(out, expected) == 0 &&
           tui_text_model_length(editor->model) == (int)strlen(expected);
}

static int vis(const TuiScrollBar *sb)
{
    return (sb->control.flags & TUI_VISIBLE) != 0;
}

static int at(TuiEditor *editor, int line, int column)
{
    TuiEditorPosition p;

    tui_editor_get_position(editor, &p);

    return p.line == line && p.column == column;
}

static void keys(Fixture *f, const char *text)
{
    while (*text != '\0')
        test_key(&f->desktop, (unsigned char)*text++);
}

static void test_ed_init(void)
{
    Fixture f;
    TuiEditorPosition p;
    TuiDesktop desktop;
    TuiEditor empty;

    ed_setup(&f, 256, 10, 4, "");
    CHECK(f.editor.model == &f.linear.model);
    CHECK(f.editor.cursor_pos == 0);
    CHECK(f.editor.top_line == 0 && f.editor.left_col == 0);
    CHECK(!tui_editor_is_modified(&f.editor));
    CHECK(f.editor.control.flags & TUI_TABSTOP);
    CHECK(f.editor.control.flags & TUI_FOCUSABLE);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 0 && p.column == 0 && p.offset == 0);
    tui_draw(&f.desktop);
    CHECK(test_cursor_visible && test_cursor_x == 2 && test_cursor_y == 2);

    ed_setup(&f, 256, 10, 4, "Hello\nWorld");
    tui_draw(&f.desktop);
    CHECK(test_cell_chars[2][2] == 'H');
    CHECK(test_cell_chars[3][6] == 'd');

    /* Without a model the editor is inert. */
    test_init_desktop(&desktop);
    tui_editor_init(&empty, 2, 2, 10, 4, 0);
    tui_add(&desktop.control, &empty.control);
    tui_desktop_set_focus(&desktop, &empty.control);
    CHECK(!test_key(&desktop, 'a'));
    tui_draw(&desktop);
    tui_editor_get_position(&empty, &p);
    CHECK(p.line == 0 && p.column == 0 && p.offset == 0);
    tui_editor_get_position(0, &p);
    CHECK(p.offset == 0);
}

static void test_ed_editing(void)
{
    Fixture f;

    ed_setup(&f, 256, 20, 5, "");
    keys(&f, "abc");
    CHECK(same(&f.editor, "abc"));
    CHECK(f.editor.cursor_pos == 3);

    CHECK(test_key(&f.desktop, TUI_KEY_ENTER));
    CHECK(same(&f.editor, "abc\n"));
    keys(&f, "de");
    CHECK(same(&f.editor, "abc\nde"));
    CHECK(at(&f.editor, 1, 2));

    /* Insert in the middle. */
    test_key(&f.desktop, TUI_KEY_LEFT);
    keys(&f, "X");
    CHECK(same(&f.editor, "abc\ndXe"));

    CHECK(test_key(&f.desktop, TUI_KEY_BACKSPACE));
    CHECK(same(&f.editor, "abc\nde"));
    CHECK(test_key(&f.desktop, TUI_KEY_DELETE));
    CHECK(same(&f.editor, "abc\nd"));

    /* Backspace joins lines, Delete too. */
    test_key(&f.desktop, TUI_KEY_HOME);
    test_key(&f.desktop, TUI_KEY_BACKSPACE);
    CHECK(same(&f.editor, "abcd"));
    CHECK(at(&f.editor, 0, 3));
    test_key(&f.desktop, TUI_KEY_ENTER);
    test_key(&f.desktop, TUI_KEY_LEFT);
    test_key(&f.desktop, TUI_KEY_DELETE);
    CHECK(same(&f.editor, "abcd"));

    /* Limits: nothing to delete. */
    test_key(&f.desktop, TUI_KEY_END);
    test_key(&f.desktop, TUI_KEY_DELETE);
    CHECK(same(&f.editor, "abcd"));
    test_key(&f.desktop, TUI_KEY_HOME);
    test_key(&f.desktop, TUI_KEY_BACKSPACE);
    CHECK(same(&f.editor, "abcd"));

    /* Unhandled keys are not consumed. */
    CHECK(!test_key(&f.desktop, TUI_KEY_F1));

    /* A full model refuses the character. */
    ed_setup(&f, 4, 20, 5, "abc");
    test_key(&f.desktop, TUI_KEY_END);
    CHECK(test_key(&f.desktop, 'x'));
    CHECK(same(&f.editor, "abc"));
    CHECK(f.editor.cursor_pos == 3);
    CHECK(!tui_editor_is_modified(&f.editor));
    test_key(&f.desktop, TUI_KEY_ENTER);
    CHECK(same(&f.editor, "abc"));

    ed_setup(&f, 1, 20, 5, "");
    keys(&f, "a");
    CHECK(same(&f.editor, ""));
}

static void test_ed_cursor(void)
{
    Fixture f;

    ed_setup(&f, 256, 20, 5, "abc\n\nlonger line\nx");

    /* Left/right cross lines. */
    test_key(&f.desktop, TUI_KEY_LEFT);
    CHECK(f.editor.cursor_pos == 0);
    test_key(&f.desktop, TUI_KEY_END);
    CHECK(at(&f.editor, 0, 3));
    test_key(&f.desktop, TUI_KEY_RIGHT);
    CHECK(at(&f.editor, 1, 0));
    test_key(&f.desktop, TUI_KEY_RIGHT);
    CHECK(at(&f.editor, 2, 0));
    test_key(&f.desktop, TUI_KEY_LEFT);
    CHECK(at(&f.editor, 1, 0));

    /* Up/down keep the column, clamped to the line. */
    test_key(&f.desktop, TUI_KEY_UP);
    test_key(&f.desktop, TUI_KEY_END);
    CHECK(at(&f.editor, 0, 3));
    test_key(&f.desktop, TUI_KEY_DOWN);
    CHECK(at(&f.editor, 1, 0));
    test_key(&f.desktop, TUI_KEY_DOWN);
    test_key(&f.desktop, TUI_KEY_END);
    CHECK(at(&f.editor, 2, 11));
    test_key(&f.desktop, TUI_KEY_DOWN);
    CHECK(at(&f.editor, 3, 1));
    test_key(&f.desktop, TUI_KEY_DOWN);
    CHECK(at(&f.editor, 3, 1));
    test_key(&f.desktop, TUI_KEY_RIGHT);
    CHECK(at(&f.editor, 3, 1));
    test_key(&f.desktop, TUI_KEY_HOME);
    CHECK(at(&f.editor, 3, 0));

    test_key(&f.desktop, TUI_KEY_PAGEUP);
    CHECK(at(&f.editor, 0, 0));
    test_key(&f.desktop, TUI_KEY_UP);
    CHECK(at(&f.editor, 0, 0));
}

static void test_ed_position(void)
{
    Fixture f;
    TuiEditorPosition p;

    /* Trailing newline: an empty last line exists. */
    ed_setup(&f, 256, 20, 5, "ab\n");
    test_key(&f.desktop, TUI_KEY_DOWN);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 1 && p.column == 0 && p.offset == 3);
    test_key(&f.desktop, TUI_KEY_UP);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 0 && p.offset == 0);

    ed_setup(&f, 256, 20, 5, "one\ntwo\n\nfour");
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 0 && p.column == 0 && p.offset == 0);
    test_key(&f.desktop, TUI_KEY_END);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 0 && p.column == 3 && p.offset == 3);
    test_key(&f.desktop, TUI_KEY_DOWN);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 1 && p.column == 3 && p.offset == 7);
    test_key(&f.desktop, TUI_KEY_DOWN);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 2 && p.column == 0 && p.offset == 8);
    test_key(&f.desktop, TUI_KEY_DOWN);
    test_key(&f.desktop, TUI_KEY_END);
    tui_editor_get_position(&f.editor, &p);
    CHECK(p.line == 3 && p.column == 4 && p.offset == 13);
    test_key(&f.desktop, TUI_KEY_HOME);
    CHECK(at(&f.editor, 3, 0));

    /* Edits change line/column. */
    test_key(&f.desktop, TUI_KEY_ENTER);
    CHECK(at(&f.editor, 4, 0));
    keys(&f, "zz");
    CHECK(at(&f.editor, 4, 2));
    test_key(&f.desktop, TUI_KEY_BACKSPACE);
    CHECK(at(&f.editor, 4, 1));
}

static void test_ed_viewport(void)
{
    Fixture f;
    int i;

    ed_setup(&f, 256, 10, 3,
             "1\n2\n3\n4\n5\n6\n7\n8\n9\n10");
    tui_draw(&f.desktop);
    CHECK(f.editor.top_line == 0);
    CHECK(test_cell_chars[2][2] == '1');

    for (i = 0; i < 4; ++i)
        test_key(&f.desktop, TUI_KEY_DOWN);

    CHECK(f.editor.top_line == 2);
    tui_draw(&f.desktop);
    CHECK(test_cell_chars[2][2] == '3');
    CHECK(test_cursor_visible && test_cursor_y == 4);

    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    CHECK(f.editor.top_line > 2);
    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    CHECK(at(&f.editor, 9, 0) || at(&f.editor, 9, 1));
    CHECK(f.editor.top_line == 7);
    test_key(&f.desktop, TUI_KEY_PAGEUP);
    CHECK(f.editor.top_line <= 6);
    test_key(&f.desktop, TUI_KEY_PAGEUP);
    test_key(&f.desktop, TUI_KEY_PAGEUP);
    test_key(&f.desktop, TUI_KEY_PAGEUP);
    CHECK(f.editor.top_line == 0 && at(&f.editor, 0, 0));

    /* Horizontal follow. */
    ed_setup(&f, 256, 10, 4, "0123456789ABCDEFGHIJ");
    test_key(&f.desktop, TUI_KEY_END);
    CHECK(f.editor.left_col > 0);
    tui_draw(&f.desktop);
    CHECK(test_cursor_visible);
    test_key(&f.desktop, TUI_KEY_HOME);
    CHECK(f.editor.left_col == 0);

    /* A shrinking resize keeps the cursor visible. */
    ed_setup(&f, 256, 20, 6, "a\nb\nc\nd\ne\nf");
    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    f.editor.control.height = 2;
    tui_draw(&f.desktop);
    CHECK(f.editor.top_line == 5 || f.editor.top_line == 4);
    CHECK(test_cursor_visible);
}

static void test_ed_scrollbars(void)
{
    Fixture f;
    TuiEvent event;

    ed_setup(&f, 256, 10, 4, "ab\ncd");
    CHECK(!vis(&f.editor.vscroll) && !vis(&f.editor.hscroll));
    CHECK(!(f.editor.vscroll.control.flags & TUI_TABSTOP));
    CHECK(!(f.editor.hscroll.control.flags & TUI_TABSTOP));
    CHECK(!(f.editor.vscroll.control.flags & TUI_FOCUSABLE));
    CHECK(f.editor.vscroll.command == TUI_CMD_NONE);
    CHECK(f.editor.hscroll.command == TUI_CMD_NONE);

    ed_setup(&f, 256, 10, 3, "1\n2\n3\n4");
    CHECK(vis(&f.editor.vscroll) && !vis(&f.editor.hscroll));
    CHECK(f.editor.vscroll.max == 4 && f.editor.vscroll.page == 3);

    /* The vertical bar steals a column, which needs the horizontal. */
    ed_setup(&f, 256, 10, 3, "0123456789\n2\n3\n4");
    CHECK(vis(&f.editor.vscroll) && vis(&f.editor.hscroll));
    CHECK(f.editor.hscroll.max == 10 && f.editor.hscroll.page == 9);
    CHECK(f.editor.vscroll.control.height == 2);
    tui_draw(&f.desktop);
    CHECK(test_cell_chars[4][11] == ' ');
    CHECK(test_cell_chars[3][11] == TUI_CH_DOWN_TRIANGLE);

    /* The horizontal bar steals a row, which needs the vertical. */
    ed_setup(&f, 256, 10, 3, "0123456789ABC\n2\n3");
    CHECK(vis(&f.editor.vscroll) && vis(&f.editor.hscroll));
    CHECK(f.editor.vscroll.page == 2);

    /* Bars drive the viewport; no cursor change, no command. */
    ed_setup(&f, 256, 10, 4, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");
    tui_draw(&f.desktop);
    CHECK(test_mouse_action(&f.desktop, 11, 5, TUI_MOUSE_DOWN, &event));
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(f.editor.top_line == 1);
    CHECK(f.editor.cursor_pos == 0);
    CHECK(!tui_editor_is_modified(&f.editor));

    f.editor.top_line = 0;
    tui_draw(&f.desktop);
    CHECK(test_mouse_action(&f.desktop, 11, 3, TUI_MOUSE_DOWN, &event));
    CHECK(f.desktop.capture == &f.editor.vscroll.control);
    CHECK(test_mouse_action(&f.desktop, 11, 4, TUI_MOUSE_MOVE, &event));
    CHECK(event.type != TUI_EV_COMMAND);
    CHECK(f.editor.top_line == 8);
    CHECK(test_mouse_action(&f.desktop, 60, 22, TUI_MOUSE_UP, &event));
    CHECK(f.desktop.capture == 0);
    CHECK(f.editor.cursor_pos == 0);
    CHECK(!tui_editor_is_modified(&f.editor));

    /* Resize makes the bars disappear. */
    ed_setup(&f, 256, 10, 4, "0123456789ABCDEFGHIJ");
    tui_draw(&f.desktop);
    CHECK(vis(&f.editor.hscroll));
    f.editor.control.width = 30;
    tui_draw(&f.desktop);
    CHECK(!vis(&f.editor.hscroll));
}

static void test_ed_mouse(void)
{
    Fixture f;
    TuiEvent event;

    ed_setup(&f, 256, 10, 3, "abc\n\nlonger\nx\ny");
    tui_draw(&f.desktop);

    CHECK(test_mouse_action(&f.desktop, 3, 2, TUI_MOUSE_DOWN, &event));
    CHECK(at(&f.editor, 0, 1));
    CHECK(event.type == TUI_EV_COMMAND &&
          event.source == &f.editor.control);

    /* Beyond the end of the line. */
    test_mouse_action(&f.desktop, 8, 2, TUI_MOUSE_DOWN, &event);
    CHECK(at(&f.editor, 0, 3));
    test_mouse_action(&f.desktop, 5, 3, TUI_MOUSE_DOWN, &event);
    CHECK(at(&f.editor, 1, 0));
    test_mouse_action(&f.desktop, 6, 4, TUI_MOUSE_DOWN, &event);
    CHECK(at(&f.editor, 2, 4));

    /* Scrolled viewport maps rows. */
    f.editor.top_line = 2;
    tui_draw(&f.desktop);
    test_mouse_action(&f.desktop, 2, 2, TUI_MOUSE_DOWN, &event);
    CHECK(at(&f.editor, 2, 0));

    /* The scroll bar strip never moves the cursor. */
    test_mouse_action(&f.desktop, 11, 2, TUI_MOUSE_DOWN, &event);
    CHECK(at(&f.editor, 2, 0));

    /* Non-left actions are ignored. */
    CHECK(!test_mouse_action(&f.desktop, 3, 2, TUI_MOUSE_MOVE, &event));
}

static void test_ed_commands(void)
{
    Fixture f;
    TuiEvent event;

    ed_setup(&f, 256, 20, 5, "ab\ncd");

    /* Movement: one command. */
    CHECK(test_key_event(&f.desktop, TUI_KEY_RIGHT, &event));
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(event.command == CMD_ED);
    CHECK(event.source == &f.editor.control);

    /* No change, no command. */
    test_key_event(&f.desktop, TUI_KEY_HOME, &event);
    CHECK(test_key_event(&f.desktop, TUI_KEY_HOME, &event));
    CHECK(event.type == TUI_EV_KEY);
    test_key_event(&f.desktop, TUI_KEY_UP, &event);
    CHECK(event.type == TUI_EV_KEY);

    /* Editing. */
    CHECK(test_key_event(&f.desktop, 'x', &event));
    CHECK(event.type == TUI_EV_COMMAND &&
          event.source == &f.editor.control);
    test_key_event(&f.desktop, TUI_KEY_DELETE, &event);
    CHECK(event.type == TUI_EV_COMMAND);
    test_key_event(&f.desktop, TUI_KEY_BACKSPACE, &event);
    CHECK(event.type == TUI_EV_COMMAND);

    /* Delete at the end of the text changes nothing. */
    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    test_key(&f.desktop, TUI_KEY_END);
    test_key_event(&f.desktop, TUI_KEY_DELETE, &event);
    CHECK(event.type == TUI_EV_KEY);

    /* Without a command nothing is produced. */
    tui_editor_set_command(&f.editor, TUI_CMD_NONE);
    test_key_event(&f.desktop, 'q', &event);
    CHECK(event.type == TUI_EV_KEY);
    CHECK(same(&f.editor, "b\ncdq"));
}

static void test_ed_modified(void)
{
    Fixture f;
    TuiEvent event;

    ed_setup(&f, 256, 10, 3, "a\nb\nc\nd\ne");
    CHECK(!tui_editor_is_modified(&f.editor));

    test_key(&f.desktop, TUI_KEY_DOWN);
    test_key(&f.desktop, TUI_KEY_END);
    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    test_mouse_action(&f.desktop, 2, 2, TUI_MOUSE_DOWN, &event);
    CHECK(!tui_editor_is_modified(&f.editor));

    test_key(&f.desktop, 'x');
    CHECK(tui_editor_is_modified(&f.editor));
    tui_editor_set_modified(&f.editor, 0);
    CHECK(!tui_editor_is_modified(&f.editor));

    test_key(&f.desktop, TUI_KEY_HOME);
    test_key(&f.desktop, TUI_KEY_DELETE);
    CHECK(tui_editor_is_modified(&f.editor));
    tui_editor_set_modified(&f.editor, 0);
    test_key(&f.desktop, TUI_KEY_BACKSPACE);
    CHECK(tui_editor_is_modified(&f.editor));
    tui_editor_set_modified(&f.editor, 5);
    CHECK(tui_editor_is_modified(&f.editor));
    tui_editor_set_modified(&f.editor, 0);

    /* Scrolling does not modify. */
    f.editor.top_line = 1;
    tui_draw(&f.desktop);
    CHECK(!tui_editor_is_modified(&f.editor));
    tui_editor_set_modified(0, 1);
    CHECK(!tui_editor_is_modified(0));
}

static void test_ed_readonly(void)
{
    Fixture f;
    TuiEvent event;

    ed_setup(&f, 256, 20, 5, "ab\ncd");
    tui_editor_set_readonly(&f.editor, 1);
    tui_draw(&f.desktop);
    CHECK(!test_cursor_visible);

    CHECK(test_key(&f.desktop, 'x'));
    CHECK(test_key(&f.desktop, TUI_KEY_ENTER));
    CHECK(test_key(&f.desktop, TUI_KEY_DELETE));
    CHECK(test_key(&f.desktop, TUI_KEY_BACKSPACE));
    CHECK(same(&f.editor, "ab\ncd"));
    CHECK(!tui_editor_is_modified(&f.editor));

    test_key_event(&f.desktop, TUI_KEY_DOWN, &event);
    CHECK(event.type == TUI_EV_COMMAND);
    CHECK(at(&f.editor, 1, 0));
}

/* A model that is not a TuiLinearTextModel: fixed text, read-only. */
static const char fake_text[] = "fake\nmodel\ntext";

static int fake_length(const TuiTextModel *model)
{
    (void)model;
    return (int)sizeof(fake_text) - 1;
}

static int fake_read(const TuiTextModel *model, int pos, char *dest,
                     int length)
{
    int n;

    if (pos < 0 || pos >= fake_length(model))
        return 0;

    n = 0;

    while (n < length && pos + n < fake_length(model)) {
        dest[n] = fake_text[pos + n];
        ++n;
    }

    return n;
}

static int fake_insert(TuiTextModel *model, int pos, const char *text,
                       int length)
{
    (void)model;
    (void)pos;
    (void)text;
    (void)length;
    return 0;
}

static int fake_delete(TuiTextModel *model, int pos, int length)
{
    (void)model;
    (void)pos;
    (void)length;
    return 0;
}

static const TuiTextModelClass fake_class = {
    fake_length, fake_read, fake_insert, fake_delete
};

static void test_ed_other_model(void)
{
    TuiDesktop desktop;
    TuiTextModel fake;
    TuiEditor editor;
    TuiEditorPosition p;
    TuiEvent event;

    fake.cls = &fake_class;
    test_init_desktop(&desktop);
    tui_editor_init(&editor, 2, 2, 10, 4, &fake);
    tui_editor_set_command(&editor, CMD_ED);
    tui_add(&desktop.control, &editor.control);
    tui_desktop_set_focus(&desktop, &editor.control);

    tui_draw(&desktop);
    CHECK(test_cell_chars[2][2] == 'f');
    CHECK(test_cell_chars[3][2] == 'm');
    CHECK(test_cell_chars[4][5] == 't');

    test_key(&desktop, TUI_KEY_DOWN);
    test_key(&desktop, TUI_KEY_END);
    tui_editor_get_position(&editor, &p);
    CHECK(p.line == 1 && p.column == 5 && p.offset == 10);

    /* Edits are refused by the model: no change, no command. */
    CHECK(test_key_event(&desktop, 'x', &event));
    CHECK(event.type == TUI_EV_KEY);
    CHECK(!tui_editor_is_modified(&editor));
    test_key_event(&desktop, TUI_KEY_ENTER, &event);
    CHECK(event.type == TUI_EV_KEY);
    test_key_event(&desktop, TUI_KEY_BACKSPACE, &event);
    CHECK(event.type == TUI_EV_KEY);
    CHECK(!tui_editor_is_modified(&editor));
}

static void test_ed_degenerate(void)
{
    Fixture f;
    TuiEvent event;
    int w;
    int h;

    for (w = 0; w <= 2; ++w) {
        for (h = 0; h <= 2; ++h) {
            ed_setup(&f, 64, w, h, "abc\ndef\nghi\njkl");
            tui_draw(&f.desktop);
            test_key(&f.desktop, TUI_KEY_DOWN);
            test_key(&f.desktop, TUI_KEY_END);
            test_key(&f.desktop, TUI_KEY_PAGEDOWN);
            test_key(&f.desktop, 'z');
            test_mouse_action(&f.desktop, 2, 2, TUI_MOUSE_DOWN, &event);
            tui_draw(&f.desktop);
            CHECK(f.editor.top_line >= 0 && f.editor.left_col >= 0);
            CHECK(f.editor.cursor_pos >= 0 &&
                  f.editor.cursor_pos <=
                  tui_text_model_length(f.editor.model));
        }
    }

    /* Empty document. */
    ed_setup(&f, 64, 5, 3, "");
    test_key(&f.desktop, TUI_KEY_PAGEDOWN);
    test_key(&f.desktop, TUI_KEY_DOWN);
    CHECK(at(&f.editor, 0, 0));
}

void test_editor_suite(void)
{
    test_run_case("editor init", test_ed_init);
    test_run_case("editor editing", test_ed_editing);
    test_run_case("editor cursor", test_ed_cursor);
    test_run_case("editor position", test_ed_position);
    test_run_case("editor viewport", test_ed_viewport);
    test_run_case("editor scrollbars", test_ed_scrollbars);
    test_run_case("editor mouse", test_ed_mouse);
    test_run_case("editor commands", test_ed_commands);
    test_run_case("editor modified", test_ed_modified);
    test_run_case("editor readonly", test_ed_readonly);
    test_run_case("editor other model", test_ed_other_model);
    test_run_case("editor degenerate sizes", test_ed_degenerate);
}
