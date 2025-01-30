#ifndef __TIMER_H__
#define __TIMER_H__

#include "stdafx.h"
#include <stdbool.h>
#include <sys/time.h>

typedef struct timer {
  double start;
  double end;
} timer;

static inline double __timer_get_currtime() {
  struct timeval tv;
  if (gettimeofday(&tv, 0))
    return __FLT_MAX__;
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// the timer will finish after 's' seconds
// pass __FLT_MAX__ for duration for it to just not end
static inline timer timer_begin(double duration) {
  if (duration <= 0.0f) {
    NV_LOG_ERROR("Timer duration passed as negative or zero. What do you even want the "
              "timer to do????");
    return (timer){-1, -1};
  }
  timer tm;
  tm.start = __timer_get_currtime();
  tm.end   = tm.start + duration;
  return tm;
}

// sets the end of the timer to -1
// resetting a timer is not required if it is going to be started again by
// timer_begin()
static inline void timer_reset(timer *tm) {
  tm->start = -1;
  tm->end   = -1;
}

static inline bool timer_is_done(const timer *tm) {
  if (tm->end == -1 || tm->start == -1) {
    NV_LOG_WARNING("Timer end is invalid (it has been reset or was not created "
                "correctly). The timer needs to be restarted using timer_begin()");
    return 0;
  }

  return __timer_get_currtime() >= tm->end;
}

static inline float timer_time_since_start(const timer *tm) {
  return (__timer_get_currtime() - tm->start);
}

static inline float timer_time_since_done(const timer *tm) {
  return (__timer_get_currtime() - tm->end);
}

#endif //__TIMER_H__