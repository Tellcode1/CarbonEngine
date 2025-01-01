#ifndef __TIMER_H__
#define __TIMER_H__

#include "defines.h"
#include <time.h>
#include <stdbool.h>

typedef struct timer {
    clock_t end;
} timer;

// the timer will finish after 's' seconds
timer timer_begin(float duration) {
    if (duration < 0.0f) {
        LOG_ERROR("Timer duration passed as negative. What do you even want the timer to do????");
        return (timer){-1};
    }
    timer tm;
    tm.end = clock() + (int)(duration * CLOCKS_PER_SEC);
    return tm;
}

// sets the end of the timer to INT32_MAX
// resetting a timer is not required if it is going to be started again by timer_begin()
void timer_reset(timer *tm) {
    tm->end = -1;
}

bool timer_is_done(const timer *tm) {
    if (tm->end == -1) {
        LOG_WARNING("Timer end is invalid (it has been reset or was not created correctly). The timer needs to be restarted using timer_begin()");
        return 0;
    }
    return clock() >= tm->end;
}

#endif//__TIMER_H__