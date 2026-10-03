#include "tui.h"

int tui_text_model_length(const TuiTextModel *model)
{
    return model->cls->length(model);
}

int tui_text_model_read(const TuiTextModel *model,
                        int pos,
                        char *dest,
                        int length)
{
    return model->cls->read(model, pos, dest, length);
}

int tui_text_model_insert(TuiTextModel *model,
                          int pos,
                          const char *text,
                          int length)
{
    return model->cls->insert(model, pos, text, length);
}

int tui_text_model_delete(TuiTextModel *model,
                          int pos,
                          int length)
{
    return model->cls->delete(model, pos, length);
}

/*
 * ------------------------------------------------------------
 * Linear text model
 * ------------------------------------------------------------
 */

static int linear_length(const TuiTextModel *model)
{
    return ((const TuiLinearTextModel *)model)->length;
}

static int linear_read(const TuiTextModel *model,
                       int pos,
                       char *dest,
                       int length)
{
    const TuiLinearTextModel *linear;
    int i;

    linear = (const TuiLinearTextModel *)model;

    if (pos < 0 || pos >= linear->length || length <= 0)
        return 0;

    if (length > linear->length - pos)
        length = linear->length - pos;

    for (i = 0; i < length; ++i)
        dest[i] = linear->buffer[pos + i];

    return length;
}

static int linear_insert(TuiTextModel *model,
                         int pos,
                         const char *text,
                         int length)
{
    TuiLinearTextModel *linear;
    int i;

    linear = (TuiLinearTextModel *)model;

    if (pos < 0 || pos > linear->length || length <= 0)
        return 0;

    /* One cell is kept for the terminator. */
    if (length > linear->capacity - 1 - linear->length)
        return 0;

    for (i = linear->length; i >= pos; --i)
        linear->buffer[i + length] = linear->buffer[i];

    for (i = 0; i < length; ++i)
        linear->buffer[pos + i] = text[i];

    linear->length += length;

    return length;
}

static int linear_delete(TuiTextModel *model,
                         int pos,
                         int length)
{
    TuiLinearTextModel *linear;
    int i;

    linear = (TuiLinearTextModel *)model;

    if (pos < 0 || pos >= linear->length || length <= 0)
        return 0;

    if (length > linear->length - pos)
        length = linear->length - pos;

    /* Move the tail down, including the terminator. */
    for (i = pos; i + length <= linear->length; ++i)
        linear->buffer[i] = linear->buffer[i + length];

    linear->length -= length;

    return length;
}

static const TuiTextModelClass linear_class = {
    linear_length,
    linear_read,
    linear_insert,
    linear_delete
};

void tui_linear_text_model_init(TuiLinearTextModel *model,
                                char *buffer,
                                int capacity)
{
    model->model.cls = &linear_class;
    model->buffer = capacity > 0 ? buffer : 0;
    model->capacity = model->buffer != 0 ? capacity : 0;
    model->length = 0;

    if (model->buffer != 0)
        model->buffer[0] = '\0';
}

void tui_linear_text_model_set_text(TuiLinearTextModel *model,
                                    const char *text)
{
    int i;

    if (model->buffer == 0)
        return;

    i = 0;

    if (text != 0) {
        while (i < model->capacity - 1 && text[i] != '\0') {
            model->buffer[i] = text[i];
            ++i;
        }
    }

    model->buffer[i] = '\0';
    model->length = i;
}
