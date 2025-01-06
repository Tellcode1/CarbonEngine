#ifndef __LUNA_STDAFX_H__
#define __LUNA_STDAFX_H__

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

// puts but with formatting and with the preceder "error". does not stop execution of program
// if you want that, use LOG_AND_ABORT instead.
void LOG_ERROR(const char * fmt, ...);
// formats the string, puts() it with the preceder "fatal error" and then aborts the program
void LOG_AND_ABORT(const char * fmt, ...);
// puts but with formatting and with the preceder "warning"
void LOG_WARNING(const char * fmt, ...);
// puts but with formatting and with the preceder "info"
void LOG_INFO(const char * fmt, ...);
// puts but with formatting and with the preceder "debug"
void LOG_DEBUG(const char * fmt, ...);

void LOG_CUSTOM(const char *preceder, const char *fmt, ...);

#define __WRAPPER1(x, y) CB_CONCAT(x, y)

// May god never have a look at this define. I will not be spared.
#define TIME_FUNCTION_REAL(func, LINE) const auto __WRAPPER1(__COUNTER_BEGIN__, __LINE__) = SDL_GetTicks64();\
							func; /* Call the function*/ \
                            LOG_DEBUG("[Line %d] Function %s took %ldms", LINE, #func, SDL_GetTicks64() - __WRAPPER1(__COUNTER_BEGIN__, __LINE__), LINE);
#define TIME_FUNCTION(func) TIME_FUNCTION_REAL(func, __LINE__)

#define DEBUG

static inline struct tm *__CG_GET_TIME() {
    time_t now;
    struct tm *tm;

    now = time(0);
    if ((tm = localtime (&now)) == NULL) {
        LOG_ERROR ("Error extracting time stuff");
        return NULL;
    }

    return tm;
}

extern void __CG_LOG(va_list args, const char *succeeder, const char *preceder, const char *str, unsigned char err);

#endif