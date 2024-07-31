#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <cstdint>
#include <cstring>

#if defined( __clang__ )
#   if __has_attribute( always_inline )
#     define CARBON_FORCE_INLINE __attribute__( ( always_inline ) ) __inline__
#   else
#     define CARBON_FORCE_INLINE inline
#   endif
#elif defined( __GNUC__ )
#   define CARBON_FORCE_INLINE __attribute__( ( always_inline ) ) __inline__
#elif defined( _MSC_VER )
#   define CARBON_FORCE_INLINE inline
#else
#   define CARBON_FORCE_INLINE inline
#endif

#if defined(__has_cpp_attribute)
    #if __has_cpp_attribute(nodiscard)
        #define CARBON_NO_DISCARD [[nodiscard]]
    #else
        #define CARBON_NO_DISCARD
    #endif
#elif defined(_MSC_VER)
    #if _MSC_VER >= 1911 // Visual Studio 2017 version 15.3
        #define CARBON_NO_DISCARD [[nodiscard]]
    #else
        #define CARBON_NO_DISCARD
    #endif
#elif defined(__GNUC__)
    #if __GNUC__ >= 7
        #define CARBON_NO_DISCARD [[nodiscard]]
    #else
        #define CARBON_NO_DISCARD
    #endif
#else
    #define CARBON_NO_DISCARD
#endif

#define CONCAT(x, y) x##y

#define array_len(arr) (sizeof(arr) / sizeof(arr[0]))

#define cassert_and_ret(expr) if (!static_cast<bool>(expr)) { LOG_ERROR("[%s : %u] Assertion %s failed", get_filename(__FILE__), __LINE__, #expr); return; }
#define cassert(expr) (!static_cast<bool>(expr) ? LOG_ERROR("[%s : %u] Assertion %s failed", get_filename(__FILE__), __LINE__, #expr) : void(0))

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

typedef uint64_t u64;
typedef uint64_t size_t;
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

constexpr static inline u32 reverse_find(const char *begin, const char *end, char value) {
    const char *iterator = end - 1;
    while (iterator >= begin) {
        if (*iterator == value)
            return iterator - begin;
        iterator--;
    }
    return (u32)-1;
}

constexpr static const char *get_filename(const char *path) {
    u32 pos = reverse_find(path, path + strlen(path), '/');
    if (pos != (u32)-1)
        return path + pos + 1;
    return path;
}

#endif