#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <cstdint>
#include <cstring>

// Reddit doin more programmin than me :>

// OS defs
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#    define CB_OS_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#    define CB_OS_LINUX
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#    define CB_OS_MACOS
#elif defined(__ANDROID__)
#    define CB_OS_ANDROID
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#    define CB_OS_BSD
#elif defined(sun) || defined(__sun)
#    if defined(__SVR4) || defined(__svr4__)
#        define CB_OS_SOLARIS
#    else
#        define CB_OS_SUNOS
#    endif
#elif defined(_AIX)
#    define CB_OS_AIX
#elif defined(hpux) || defined(__hpux)
#    define CB_OS_HPUX
#elif defined(sgi) || defined(__sgi)
#    define CB_OS_IRIX
#elif defined(__HAIKU__)
#    define CB_OS_HAIKU
#elif defined(__BEOS__)
#    define CB_OS_BEOS
#elif defined(__QNX__)
#    define CB_OS_QNX
#elif defined(__minix)
#    define CB_OS_MINIX
#elif defined(__VMS)
#    define CB_OS_VMS
#else
#    error OS Unknown. Define CB_OS_* to force it through.
#endif
// OS defs


// Compiler
#if defined(__clang__)
    #define CB_COMPILER_CLANG
    #define CB_COMPILER_CLANG_VERSION __clang_version__
#elif defined(__ICC) || defined(__INTEL_CB_COMPILER)
    #define CB_COMPILER_INTEL
    #define CB_COMPILER_INTEL_VERSION __INTEL_CB_COMPILER
#elif defined(__GNUC__) || defined(__GNUG__)
    #define CB_COMPILER_GCC
    #define CB_COMPILER_GCC_VERSION __VERSION__
#elif defined(_MSC_VER)
    #define CB_COMPILER_MSVC
    #define CB_COMPILER_MSVC_VERSION _MSC_VER
#elif defined(__EMSCRIPTEN__)
    #define CB_COMPILER_EMSCRIPTEN
    #define CB_COMPILER_EMSCRIPTEN_VERSION (__EMSCRIPTEN_major__ * 10000 + __EMSCRIPTEN_minor__ * 100 + __EMSCRIPTEN_tiny__)
#elif defined(__MINGW32__) || defined(__MINGW64__)
    #define CB_COMPILER_MINGW
    #define CB_COMPILER_MINGW_VERSION (__MINGW32_VERSION_MAJOR * 10000 + __MINGW32_VERSION_MINOR * 100)
#elif defined(__NVCC__)
    #define CB_COMPILER_NVCC
    #define CB_COMPILER_NVCC_VERSION (__CUDACC_VER_MAJOR__ * 10000 + __CUDACC_VER_MINOR__ * 100 + __CUDACC_VER_BUILD__)
#elif defined(__PGI)
    #define CB_COMPILER_PGI
    #define CB_COMPILER_PGI_VERSION (__PGIC__ * 10000 + __PGIC_MINOR__ * 100 + __PGIC_PATCHLEVEL__)
#else
    #define CB_COMPILER_UNKNOWN
#endif
// Compiler


// FORCE_INLINE
#if defined(CB_COMPILER_CLANG)
#   if __has_attribute( always_inline )
#     define CARBON_FORCE_INLINE __attribute__( ( always_inline ) ) __inline__
#   else
#     define CARBON_FORCE_INLINE inline
#   endif
#elif defined(CB_COMPILER_GCC)
#   define CARBON_FORCE_INLINE __attribute__( ( always_inline ) ) __inline__
#elif defined(CB_COMPILER_MSVC)
#   define CARBON_FORCE_INLINE inline
#else
#   define CARBON_FORCE_INLINE inline
#endif
// FORCE_INLINE


// NO_DISCARD
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
// NO_DISCARD


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

// These aren't guranteed to be 32/64 bits. (I think so, I may be wrong)
typedef float f32;
typedef double f64;

constexpr static inline u32 reverse_find(const char *begin, const char *end, char value) {
    const char *iterator = end - 1;
    while (iterator >= begin) {
        if (*iterator == value)
            return iterator - begin;
        iterator--;
    }
    return UINT32_MAX;
}

constexpr static const char *get_filename(const char *path) {
    u32 pos = reverse_find(path, path + strlen(path), '/');
    if (pos != UINT32_MAX)
        return path + pos + 1;
    return path;
}

#endif