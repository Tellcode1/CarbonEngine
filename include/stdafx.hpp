#ifndef __PCH__
#define __PCH__

// #define DEBUG

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

using glm::mat2;
using glm::mat3;
using glm::mat4;

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <ctime>
#include <thread>
#include <iomanip>

#include <boost/functional/hash.hpp>

#include <memory_resource>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <chrono>
#define CONCAT_REAL(x, y) x##y
#define CONCAT(x, y) CONCAT_REAL(x, y)
#define TIME_FUNCTION_REAL(func, LINE) const auto CONCAT(__COUNTER_BEGIN__, __LINE__) = std::chrono::high_resolution_clock::now();\
							func; /* Call the function*/ \
							std::cout << #func << " Took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - CONCAT(__COUNTER_BEGIN__, __LINE__)).count() << "ms\n"

#define TIME_FUNCTION(func) TIME_FUNCTION_REAL(func, __LINE__)

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#define DEBUG

#include "AssetLoader.hpp"
#include "pro.hpp"

const std::vector<const char*> ValidationLayers = {  
	"VK_LAYER_KHRONOS_validation",
};
const std::vector<const char*> InstanceExtensions = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};
const std::vector<const char*> DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#endif