#ifndef __C_SIGNAL_HPP__
#define __C_SIGNAL_HPP__

#include "cvector.hpp"
#include "defines.h"

typedef void (* csignal_empty_func)(void);

struct csignal : cvector<csignal_empty_func>
{
    CARBON_FORCE_INLINE void signal(void) {
        for (const csignal_empty_func *iter = begin(); iter != end(); iter++)
            (*iter)();
    }

    CARBON_FORCE_INLINE void connect(csignal_empty_func func) {
        this->push_back(func);
    }
};

#endif