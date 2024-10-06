#ifndef __CSTRING_H
#define __CSTRING_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct cstring_t cstring_t;

/*
    initial_size may be 0
*/
extern cstring_t *cstring_init(int initial_size);
extern cstring_t *cstring_init_str(const char *init);
extern cstring_t *cstring_substring(const cstring_t *str, int start, int length);
extern void cstring_destroy(cstring_t *str);

extern void cstring_clear(cstring_t *str);
extern int cstring_length(const cstring_t *str);
extern int cstring_capacity(const cstring_t *str);
extern const char *cstring_data(const cstring_t *str);

extern void cstring_append(cstring_t *str, const char *suffix);
extern void cstring_append_char(cstring_t *str, char suffix);
extern void cstring_prepend(cstring_t *str, const char *prefix);
extern void cstring_set(cstring_t *str, const char *new_str);

/* returns -1 on no find */
extern int cstring_find(const cstring_t *str, const char *substr);
extern void cstring_remove(cstring_t *str, int index, int length);

extern void cstring_copy_from(const cstring_t *src, cstring_t *dst);
// src is destroyed and unusable after this call!
extern void cstring_move_from(cstring_t *src, cstring_t *dst);

#ifdef __cplusplus
    }
#endif

#endif // __CSTRING_H