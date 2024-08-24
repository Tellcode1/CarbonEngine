#ifndef __PCH__
#define __PCH__

#include "defines.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/gtc/matrix_transform.hpp>
using glm::mat4;

#include <iostream>

#define __WRAPPER1(x, y) CONCAT(x, y)

// May god never have a look at this define. I will not be spared.
#define TIME_FUNCTION_REAL(func, LINE) const auto __WRAPPER1(__COUNTER_BEGIN__, __LINE__) = SDL_GetTicks64();\
							func; /* Call the function*/ \
                            LOG_DEBUG("[Line %d] Function %s took %ldms", LINE, #func, SDL_GetTicks64() - __WRAPPER1(__COUNTER_BEGIN__, __LINE__), LINE);
#define TIME_FUNCTION(func) TIME_FUNCTION_REAL(func, __LINE__)

#define DEBUG

#include <stdarg.h>
#define __LOG()     va_list args; \
                    va_start(args, fmt); \
                    vfprintf(stderr, (preceder + fmt + succeeder).c_str(), args); \
                    va_end(args)

inline void LOG_ERROR(std::string fmt, ...) {
    const std::string preceder = "[ERROR] ";
    const std::string succeeder = "\n";
    __LOG();
}

inline void LOG_AND_ABORT(std::string fmt, ...) {
    const std::string preceder = ANSI_FORMAT_RED + std::string("[FATAL ERROR] ") + ANSI_FORMAT_RESET;
    const std::string succeeder = "\nThe program can not continue\n";
    __LOG();
    abort();
}

inline void LOG_WARNING(std::string fmt, ...) {
    const std::string preceder = ANSI_FORMAT_YELLOW + std::string("[Warning] ") + ANSI_FORMAT_RESET;
    const std::string succeeder = "\n";
    __LOG();
}

inline void LOG_INFO(std::string fmt, ...) {
    const std::string preceder = ANSI_FORMAT_CYAN + std::string("[Info] ") + ANSI_FORMAT_RESET;
    const std::string succeeder = "\n";
    __LOG();
}

inline void LOG_DEBUG(std::string fmt, ...) {
    const std::string preceder = ANSI_FORMAT_GREEN + std::string("[Debug] ") + ANSI_FORMAT_RESET;
    const std::string succeeder = "\n";
    __LOG();
}

#undef __LOG

inline u32 align(u32 size, u32 alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

#endif