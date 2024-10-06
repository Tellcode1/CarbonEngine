// I wanted to reuse cvector.h but nah.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/defines.h"
#include "../../include/containers/cstring.h"

struct cstring_t {
    char *data;
    int length;
    int capacity;
};

static void cstring_resize(cstring_t *str, int new_capacity) {
    str->data = realloc(str->data, new_capacity);
    cassert(str->data != NULL);

    str->capacity = new_capacity;
}

cstring_t *cstring_init(int initial_size) {
    cstring_t *str = malloc(sizeof(cstring_t));
    cassert(str != NULL);
    
    str->capacity = initial_size > 0 ? initial_size : 1;
    str->data = malloc(str->capacity);
    cassert(str->data != NULL);
    
    str->data[0] = '\0';
    str->length = 0;
    return str;
}

cstring_t *cstring_init_str(const char *init)
{
    cassert(init != NULL && strlen(init) > 0);

    cstring_t *str = malloc(sizeof(cstring_t));
    cassert(str != NULL);
    
    int len = strlen(init);
    str->capacity = len + 1;
    str->data = malloc(str->capacity);
    cassert(str->data != NULL);
    
    strcpy(str->data, init);
    str->length = len;
    
    return str;
}

cstring_t *cstring_substring(const cstring_t *str, int start, int length)
{
    cstring_t *substr = cstring_init(length + 1);
    cassert(substr != NULL);

    strncpy(substr->data, str->data + start, length);
    substr->data[length] = '\0';
    substr->length = length;
    return substr;
}

void cstring_destroy(cstring_t *str)
{
    if (str) {
        free(str->data);
        free(str);
    }
}

void cstring_clear(cstring_t *str) {
    if (str) {
        str->length = 0;
        str->data[0] = '\0';
    }
}

int cstring_length(const cstring_t *str) {
    return str ? str->length : 0;
}

int cstring_capacity(const cstring_t *str) {
    return str ? str->capacity : 0;
}

const char *cstring_data(const cstring_t *str) {
    return str ? str->data : NULL;
}

void cstring_append(cstring_t *str, const char *suffix) {
    int suffix_length = strlen(suffix);
    if (str->length + suffix_length + 1 > str->capacity) {
        cstring_resize(str, str->length + suffix_length + 1);
    }

    strcpy(str->data + str->length, suffix);
    str->length += suffix_length;
}

void cstring_append_char(cstring_t *str, char suffix)
{
    if (str->length + 2 > str->capacity) {
        cstring_resize(str, str->length + 2);
    }

    str->data[str->length] = suffix;
    str->length++;
}

void cstring_prepend(cstring_t *str, const char *prefix) {
    if (!str || !prefix) return;

    int prefix_length = strlen(prefix);
    if (str->length + prefix_length + 1 > str->capacity) {
        cstring_resize(str, str->length + prefix_length + 1);
    }

    memmove(str->data + prefix_length, str->data, str->length + 1);
    memcpy(str->data, prefix, prefix_length);
    str->length += prefix_length;
}

void cstring_set(cstring_t *str, const char *new_str) {
    if (!str || !new_str) return;

    int new_length = strlen(new_str);
    if (new_length + 1 > str->capacity) {
        cstring_resize(str, new_length + 1);
    }

    strcpy(str->data, new_str);
    str->length = new_length;
}

int cstring_find(const cstring_t *str, const char *substr) {
    if (!str || !substr) return -1;

    char *pos = strstr(str->data, substr);
    return pos ? (int)(pos - str->data) : -1;
}

void cstring_remove(cstring_t *str, int index, int length) {
    if (!str || index < 0 || index >= str->length) return;

    if (index + length > str->length) {
        length = str->length - index;
    }

    memmove(str->data + index, str->data + index + length, str->length - index - length + 1);
    str->length -= length;
}

void cstring_copy_from(const cstring_t *src, cstring_t *dst) {
    cstring_set(dst, src->data);
}

void cstring_move_from(cstring_t *src, cstring_t *dst) {
    cstring_copy_from(src, dst);
    cstring_destroy(src);
}