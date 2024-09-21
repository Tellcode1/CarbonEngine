#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdint.h>
#include <string.h>

// Reddit doin more programmin than me :>

// OS defs
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#    define CB_OS_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#    define CB_OS_LINUX
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#    define CB_OS_MAC
#elif defined(__ANDROID__)
#    define CB_OS_ANDROID
#else
#    error Unknown OS. Only Windows, Linux, Mac and android are supported.
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
    #warning Unknown compiler
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


#define CB_CONCAT(x, y) x##y

#define array_len(arr) (sizeof(arr) / sizeof(arr[0]))

#define cassert_and_ret(expr) if (!static_cast<bool>(expr)) { LOG_ERROR("[%s : %u] Assertion %s failed", basename(__FILE__), __LINE__, #expr); return; }
#define cassert(expr) (!static_cast<bool>(expr) ? LOG_ERROR("[%s : %u] Assertion %s failed", basename(__FILE__), __LINE__, #expr) : void(0))

typedef uint64_t u64;
typedef uint64_t size_t; // vscode somtimes complains so I just have this here.
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

#endif