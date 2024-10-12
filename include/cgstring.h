#ifndef __cg_string_H
#define __cg_string_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct cg_string_t cg_string_t;

/*
    initial_size may be 0
*/
extern cg_string_t *cg_string_init(int initial_size);
extern cg_string_t *cg_string_init_str(const char *init);
extern cg_string_t *cg_string_init_ptr(const char *begin, const char *end);
extern cg_string_t *cg_string_substring(const cg_string_t *str, int start, int length);
extern void cg_string_destroy(cg_string_t *str);

extern void cg_string_clear(cg_string_t *str);
extern int cg_string_length(const cg_string_t *str);
extern int cg_string_capacity(const cg_string_t *str);
extern const char *cg_string_data(const cg_string_t *str);

extern void cg_string_append(cg_string_t *str, const char *suffix);
extern void cg_string_append_char(cg_string_t *str, char suffix);
extern void cg_string_prepend(cg_string_t *str, const char *prefix);
extern void cg_string_set(cg_string_t *str, const char *new_str);

/* returns -1 on no find */
extern int cg_string_find(const cg_string_t *str, const char *substr);
extern void cg_string_remove(cg_string_t *str, int index, int length);

extern void cg_string_copy_from(const cg_string_t *src, cg_string_t *dst);
// src is destroyed and unusable after this call!
extern void cg_string_move_from(cg_string_t *src, cg_string_t *dst);

#ifdef __cplusplus
    }
#endif

#endif // __cg_string_H