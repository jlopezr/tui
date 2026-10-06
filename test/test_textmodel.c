#include <string.h>

#include "test_support.h"

static const char *model_text(TuiLinearTextModel *m, char *out)
{
    int n;

    n = tui_text_model_read(&m->model, 0, out, 64);
    out[n] = '\0';

    return out;
}

static int model_is(TuiLinearTextModel *m, const char *expected)
{
    char out[64];

    return strcmp(model_text(m, out), expected) == 0 &&
           tui_text_model_length(&m->model) == (int)strlen(expected);
}

static void test_tm_basic(void)
{
    char buf[16];
    char out[16];
    TuiLinearTextModel m;
    TuiLinearTextModel none;

    tui_linear_text_model_init(&m, buf, 16);
    CHECK(tui_text_model_length(&m.model) == 0);
    CHECK(tui_text_model_read(&m.model, 0, out, 4) == 0);
    CHECK(model_is(&m, ""));
    CHECK(buf[0] == '\0');

    tui_linear_text_model_set_text(&m, "hello");
    CHECK(tui_text_model_length(&m.model) == 5);
    CHECK(tui_text_model_read(&m.model, 0, out, 5) == 5);
    CHECK(memcmp(out, "hello", 5) == 0);

    /* Partial and clamped reads. */
    CHECK(tui_text_model_read(&m.model, 1, out, 3) == 3);
    CHECK(memcmp(out, "ell", 3) == 0);
    CHECK(tui_text_model_read(&m.model, 3, out, 10) == 2);
    CHECK(memcmp(out, "lo", 2) == 0);
    CHECK(tui_text_model_read(&m.model, 5, out, 1) == 0);
    CHECK(tui_text_model_read(&m.model, -1, out, 1) == 0);
    CHECK(tui_text_model_read(&m.model, 0, out, 0) == 0);

    /* Truncating set_text keeps the terminator. */
    tui_linear_text_model_set_text(&m, "0123456789ABCDEFGHIJ");
    CHECK(tui_text_model_length(&m.model) == 15);
    CHECK(buf[15] == '\0');
    tui_linear_text_model_set_text(&m, 0);
    CHECK(model_is(&m, ""));

    /* No buffer: everything is a no-op. */
    tui_linear_text_model_init(&none, 0, 16);
    CHECK(tui_text_model_length(&none.model) == 0);
    CHECK(tui_text_model_insert(&none.model, 0, "a", 1) == 0);
    CHECK(tui_text_model_delete(&none.model, 0, 1) == 0);
    tui_linear_text_model_set_text(&none, "x");
    CHECK(tui_text_model_length(&none.model) == 0);
}

static void test_tm_insert(void)
{
    char buf[16];
    TuiLinearTextModel m;

    tui_linear_text_model_init(&m, buf, 16);

    CHECK(tui_text_model_insert(&m.model, 0, "bc", 2) == 2);
    CHECK(model_is(&m, "bc"));
    CHECK(tui_text_model_insert(&m.model, 0, "a", 1) == 1);
    CHECK(model_is(&m, "abc"));
    CHECK(tui_text_model_insert(&m.model, 3, "e", 1) == 1);
    CHECK(model_is(&m, "abce"));
    CHECK(tui_text_model_insert(&m.model, 3, "d", 1) == 1);
    CHECK(model_is(&m, "abcde"));
    CHECK(strcmp(buf, "abcde") == 0);

    /* Invalid offsets and lengths. */
    CHECK(tui_text_model_insert(&m.model, -1, "x", 1) == 0);
    CHECK(tui_text_model_insert(&m.model, 6, "x", 1) == 0);
    CHECK(tui_text_model_insert(&m.model, 0, "x", 0) == 0);
    CHECK(model_is(&m, "abcde"));

    /* Capacity: all or nothing, one cell reserved. */
    CHECK(tui_text_model_insert(&m.model, 5, "0123456789", 10) == 10);
    CHECK(tui_text_model_length(&m.model) == 15);
    CHECK(tui_text_model_insert(&m.model, 0, "x", 1) == 0);
    CHECK(model_is(&m, "abcde0123456789"));
    CHECK(buf[15] == '\0');

    tui_linear_text_model_init(&m, buf, 4);
    CHECK(tui_text_model_insert(&m.model, 0, "abcd", 4) == 0);
    CHECK(tui_text_model_insert(&m.model, 0, "abc", 3) == 3);
    CHECK(tui_text_model_insert(&m.model, 0, "x", 1) == 0);
    CHECK(model_is(&m, "abc"));

    tui_linear_text_model_init(&m, buf, 1);
    CHECK(tui_text_model_insert(&m.model, 0, "a", 1) == 0);
    CHECK(model_is(&m, ""));
}

/* adopt keeps the C string already in the buffer, cut inside the capacity. */
static void test_tm_adopt(void)
{
    char buf[16];
    TuiLinearTextModel m;

    strcpy(buf, "Hello\nWorld");
    tui_linear_text_model_adopt(&m, buf, 16);
    CHECK(tui_text_model_length(&m.model) == 11);
    CHECK(model_is(&m, "Hello\nWorld"));
    CHECK(tui_text_model_insert(&m.model, 5, "!", 1) == 1);
    CHECK(model_is(&m, "Hello!\nWorld"));

    /* No terminator inside the capacity: cut there. */
    memset(buf, 'x', sizeof(buf));
    tui_linear_text_model_adopt(&m, buf, 4);
    CHECK(buf[3] == '\0');
    CHECK(tui_text_model_length(&m.model) == 3);

    /* A capacity of one holds nothing; no buffer, nothing at all. */
    tui_linear_text_model_adopt(&m, buf, 1);
    CHECK(tui_text_model_length(&m.model) == 0 && buf[0] == '\0');
    tui_linear_text_model_adopt(&m, 0, 16);
    CHECK(tui_text_model_length(&m.model) == 0);
    CHECK(tui_text_model_insert(&m.model, 0, "a", 1) == 0);
}

static void test_tm_delete(void)
{
    char buf[16];
    TuiLinearTextModel m;

    tui_linear_text_model_init(&m, buf, 16);
    tui_linear_text_model_set_text(&m, "abcdefgh");

    CHECK(tui_text_model_delete(&m.model, 0, 1) == 1);
    CHECK(model_is(&m, "bcdefgh"));
    CHECK(tui_text_model_delete(&m.model, 2, 2) == 2);
    CHECK(model_is(&m, "bcfgh"));
    CHECK(tui_text_model_delete(&m.model, 4, 1) == 1);
    CHECK(model_is(&m, "bcfg"));
    CHECK(strcmp(buf, "bcfg") == 0);

    /* Clamped at the end. */
    CHECK(tui_text_model_delete(&m.model, 2, 10) == 2);
    CHECK(model_is(&m, "bc"));

    /* Invalid offsets and lengths. */
    CHECK(tui_text_model_delete(&m.model, 2, 1) == 0);
    CHECK(tui_text_model_delete(&m.model, -1, 1) == 0);
    CHECK(tui_text_model_delete(&m.model, 0, 0) == 0);
    CHECK(tui_text_model_delete(&m.model, 0, -3) == 0);
    CHECK(model_is(&m, "bc"));

    CHECK(tui_text_model_delete(&m.model, 0, 2) == 2);
    CHECK(model_is(&m, ""));
    CHECK(buf[0] == '\0');
    CHECK(tui_text_model_delete(&m.model, 0, 1) == 0);
}

void test_textmodel_suite(void)
{
    test_run_case("textmodel basic", test_tm_basic);
    test_run_case("textmodel insert", test_tm_insert);
    test_run_case("textmodel delete", test_tm_delete);
    test_run_case("textmodel adopt", test_tm_adopt);
}
