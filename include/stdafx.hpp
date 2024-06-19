#ifndef __PCH__
#define __PCH__

#if defined( __clang__ )
#   if __has_attribute( always_inline )
#     define NANO_INLINE __attribute__( ( always_inline ) ) __inline__
#   else
#     define NANO_INLINE inline
#   endif
#elif defined( __GNUC__ )
#   define NANO_INLINE __attribute__( ( always_inline ) ) __inline__
#elif defined( _MSC_VER )
#   define NANO_INLINE inline
#else
#   define NANO_INLINE inline
#endif

#define NANO_SINGLETON_FUNCTION static NANO_INLINE

#include <cstdint>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t uchar;
typedef int8_t sbyte;
typedef uint8_t ubyte;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef float f32;
typedef double f64;

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// #define GLM_ENABLE_EXPERIMENTAL

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

#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

constexpr const char* ANSI_FORMAT_BLACK = "\033[30m";
constexpr const char* ANSI_FORMAT_RED = "\033[31m";
constexpr const char* ANSI_FORMAT_GREEN = "\033[32m";
constexpr const char* ANSI_FORMAT_YELLOW = "\033[33m";
constexpr const char* ANSI_FORMAT_BLUE = "\033[34m";
constexpr const char* ANSI_FORMAT_MAGENTA = "\033[35m";
constexpr const char* ANSI_FORMAT_CYAN = "\033[36m";
constexpr const char* ANSI_FORMAT_WHITE = "\033[37m";
constexpr const char* ANSI_FORMAT_RESET = "\033[0m";
constexpr const char* ANSI_FORMAT_DEFAULT = ANSI_FORMAT_RESET;

#include <iostream>
#include <iomanip>
#include <fstream>

#include <ctime>
#include <thread>
#include <atomic>
#include <mutex>

#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <vulkan/vulkan.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <chrono>
#define TIME_FUNCTION_REAL_REAL_REAL(x, y) x##y
#define TIME_FUNCTION_REAL_REAL(x, y) TIME_FUNCTION_REAL_REAL_REAL(x, y)
#define TIME_FUNCTION_REAL(func, LINE) const auto TIME_FUNCTION_REAL_REAL(__COUNTER_BEGIN__, __LINE__) = std::chrono::high_resolution_clock::now();\
							func; /* Call the function*/ \
							std::cout << #func << " Took " << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - TIME_FUNCTION_REAL_REAL(__COUNTER_BEGIN__, __LINE__)).count() / 1000000.0f << "ms\n"

#define TIME_FUNCTION(func) TIME_FUNCTION_REAL(func, __LINE__)

#include <boost/signals2.hpp>

#define DEBUG

inline void LOG_ERROR(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, fmt, args);
    va_end(args);

}

inline void LOG_AND_ABORT(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, fmt, args);
    abort();
    va_end(args);
}

#define array_len(arr) (sizeof(arr) / sizeof(arr[0]))

#include "pro.hpp"
#include "Context.hpp"
#include "Renderer.hpp"

#endif