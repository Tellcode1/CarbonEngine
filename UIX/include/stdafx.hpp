#ifndef __STDAFX_HPP__
#define __STDAFX_HPP__

#include <cstdint>

// My eyes just love seeing these instead of the defaults.

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t uchar;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef float f32;
typedef double f64;

#define GLM_FORCE_ROW_MAJOR
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>

using glm::vec2;
using glm::ivec2;
using glm::uvec2;
using glm::dvec2;

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <fstream>

#include <ft2build.h>
#include FT_FREETYPE_H

#endif //__STDAFX_HPP__