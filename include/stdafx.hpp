#ifndef __PCH__
#define __PCH__

#include "defines.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using glm::vec2;
using glm::ivec2;
using glm::uvec2;
using glm::dvec2;

using glm::vec3;
using glm::ivec3;
using glm::uvec3;
using glm::dvec3;

using glm::vec4;
using glm::ivec4;
using glm::uvec4;
using glm::dvec4;

typedef glm::vec<2, i16> i16vec2;
typedef glm::vec<3, i16> i16vec3;
typedef glm::vec<4, i16> i16vec4;

typedef glm::vec<2, i8> i8vec2;
typedef glm::vec<3, i8> i8vec3;
typedef glm::vec<4, i8> i8vec4;

// A vec2 of u16's, etc.
typedef glm::vec<2, u16> u16vec2;
typedef glm::vec<3, u16> u16vec3;
typedef glm::vec<4, u16> u16vec4;

typedef glm::vec<2, u8> u8vec2;
typedef glm::vec<3, u8> u8vec3;
typedef glm::vec<4, u8> u8vec4;

using glm::mat2;
using glm::mat3;
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