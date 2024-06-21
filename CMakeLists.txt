cmake_minimum_required(VERSION 3.25)
project(main)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIRS})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

find_package(Boost REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})

# find_package(stb REQUIRED)
# include_directories(${stb_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(OUTPUT
    main
)

include_directories(${CMAKE_SOURCE_DIR}/Atlas)

set(SOURCES
    src/pro.cpp
    src/main.cpp
    src/Bootstrap.cpp
    src/Renderer.cpp
    src/CFTextRenderer.cpp
    src/epichelperlib.cpp
    src/CFont.cpp
)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/Atlas)
include_directories(${CMAKE_SOURCE_DIR}/UIX)
include_directories(${CMAKE_SOURCE_DIR}/build)

set(HAS_MSDF 1)

set(MSDFGEN_CORE_ONLY OFF)
set(MSDFGEN_BUILD_STANDALONE OFF)
set(MSDFGEN_USE_SKIA OFF)
set(MSDFGEN_USE_VCPKG OFF)

set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/tinyxml2)
include_directories(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen/msdfgen)
include_directories(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen/msdf-atlas-gen)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen)

set(LIBS
    vulkan
    SDL3
    freetype
    ${Boost_LIBRARIES}
    msdf-atlas-gen
    # stb
)

exec_program(
    ${CMAKE_SOURCE_DIR}/Shaders/compile.sh
    ${CMAKE_SOURCE_DIR}/Shaders
    ARGS ${CMAKE_BINARY_DIR}/ # Provide binary directory to app for placing output
)

add_executable(${OUTPUT} ${SOURCES})
target_link_options(${OUTPUT} PRIVATE -g -Wall -fopenmp)
target_compile_options(${OUTPUT} PRIVATE -g -Wall -fopenmp)
target_link_libraries(${OUTPUT} ${LIBS})

# add_executable(Atlas ${ATLAS_SOURCES})
# target_link_libraries(Atlas ${ATLAS_LIBS})

target_precompile_headers(${OUTPUT} PRIVATE include/stdafx.hpp)