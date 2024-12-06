#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/defines.h"
#include "../include/cgstring.h"

#include "../include/cgcontstdafx.h"

static void cg_string_resize(cg_string_t *str, int new_capacity) {
    char *new_data = cg_cont_realloc(str->data, new_capacity);
    cassert(new_data != NULL);
    str->data = new_data;
    str->capacity = new_capacity;
}

cg_string_t *cg_string_init(int initial_size) {
    cg_string_t *str = cg_cont_alloc(sizeof(cg_string_t));
    cassert(str != NULL);
    
    str->capacity = (initial_size > 0) ? initial_size : 1;
    str->data = cg_cont_alloc(str->capacity);
    cassert(str->data != NULL);
    
    str->data[0] = '\0';
    str->length = 0;
    return str;
}

cg_string_t *cg_string_init_str(const char *init) {
    cassert(init != NULL && strlen(init) > 0);
    cg_string_t *str = cg_cont_alloc(sizeof(cg_string_t));
    cassert(str != NULL);
    
    int len = strlen(init);
    str->capacity = len + 1;
    str->data = cg_cont_alloc(str->capacity);
    cassert(str->data != NULL);
    
    strcpy(str->data, init);
    str->length = len;
    
    return str;
}

cg_string_t *cg_string_substring(const cg_string_t *str, int start, int length) {
    cassert(str != NULL);
    cassert(start >= 0 && start + length <= str->length);

    cg_string_t *substr = cg_string_init(length + 1);
    cassert(substr != NULL);

    strncpy(substr->data, str->data + start, length);
    substr->data[length] = '\0';
    substr->length = length;
    return substr;
}

void cg_string_destroy(cg_string_t *str) {
    if (str) {
        cg_cont_free(str->data);
        cg_cont_free(str);
    }
}

void cg_string_clear(cg_string_t *str) {
    if (str) {
        str->length = 0;
        str->data[0] = '\0';
    }
}

int cg_string_length(const cg_string_t *str) {
    cassert(str != NULL);
    return str->length;
}

int cg_string_capacity(const cg_string_t *str) {
    cassert(str != NULL);
    return str->capacity;
}

const char *cg_string_data(const cg_string_t *str) {
    cassert(str != NULL);
    return str->data;
}

void cg_string_append(cg_string_t *str, const char *suffix) {
    cassert(str != NULL);
    cassert(suffix != NULL);
    
    int suffix_length = strlen(suffix);
    if (str->length + suffix_length + 1 > str->capacity) {
        cg_string_resize(str, str->length + suffix_length + 1);
    }

    strcpy(str->data + str->length, suffix);
    str->length += suffix_length;
}

void cg_string_append_char(cg_string_t *str, char suffix) {
    cassert(str != NULL);
    
    if (str->length + 2 > str->capacity) {
        cg_string_resize(str, str->length + 2);
    }

    str->data[str->length] = suffix;
    str->length++;
    str->data[str->length] = '\0';
}

void cg_string_prepend(cg_string_t *str, const char *prefix) {
    cassert(str != NULL);
    cassert(prefix != NULL);

    int prefix_length = strlen(prefix);
    if (str->length + prefix_length + 1 > str->capacity) {
        cg_string_resize(str, str->length + prefix_length + 1);
    }

    memmove(str->data + prefix_length, str->data, str->length + 1);
    memcpy(str->data, prefix, prefix_length);
    str->length += prefix_length;
}

void cg_string_set(cg_string_t *str, const char *new_str) {
    cassert(str != NULL);
    cassert(new_str != NULL);

    int new_length = strlen(new_str);
    if (new_length + 1 > str->capacity) {
        cg_string_resize(str, new_length + 1);
    }

    strcpy(str->data, new_str);
    str->length = new_length;
}

int cg_string_find(const cg_string_t *str, const char *substr) {
    cassert(str != NULL);
    cassert(substr != NULL);

    char *pos = strstr(str->data, substr);
    return pos ? (int)(pos - str->data) : -1;
}

void cg_string_remove(cg_string_t *str, int index, int length) {
    cassert(str != NULL);
    cassert(index >= 0 && index < str->length);

    if (index + length > str->length) {
        length = str->length - index;
    }

    memmove(str->data + index, str->data + index + length, str->length - index - length + 1);
    str->length -= length;
}

void cg_string_copy_from(const cg_string_t *src, cg_string_t *dst) {
    cassert(src != NULL);
    cassert(dst != NULL);
    cg_string_set(dst, src->data);
}

void cg_string_move_from(cg_string_t *src, cg_string_t *dst) {
    cassert(src != NULL);
    cassert(dst != NULL);
    cg_string_copy_from(src, dst);
    cg_string_destroy(src);
}