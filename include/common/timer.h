#ifndef __TIMER_H__
#define __TIMER_H__

#include "stdafx.h"
#include <time.h>
#include <stdbool.h>

typedef struct timer {
    clock_t start;
    clock_t end;
} timer;

// the timer will finish after 's' seconds
// pass __FLT_MAX__ for duration for it to just not end
static inline timer timer_begin(float duration) {
    if (duration < 0.0f) {
        LOG_ERROR("Timer duration passed as negative. What do you even want the timer to do????");
        return (timer){-1,-1};
    }
    timer tm;
    tm.start = clock();
    tm.end = clock() + (int)(duration * CLOCKS_PER_SEC);
    return tm;
}

// sets the end of the timer to INT32_MAX
// resetting a timer is not required if it is going to be started again by timer_begin()
static inline void timer_reset(timer *tm) {
    tm->start = -1;
    tm->end = -1;
}

static inline bool timer_is_done(const timer *tm) {
    if (tm->end == -1 || tm->start == -1) {
        LOG_WARNING("Timer end is invalid (it has been reset or was not created correctly). The timer needs to be restarted using timer_begin()");
        return 0;
    }
    return clock() >= tm->end;
}

static inline float timer_time_since_start(const timer *tm) {
    return (clock() - tm->start) / (float)CLOCKS_PER_SEC;
}

static inline float timer_time_since_done(const timer *tm) {
    return (clock() - tm->end) / (float)CLOCKS_PER_SEC;
}

#endif//__TIMER_H__