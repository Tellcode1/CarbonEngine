cmake_minimum_required(VERSION 3.15)
project(main)
# project(Atlas)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIRS})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

# find_package(STB REQUIRED)
# include_directories(${STB_INCLUDE_DIRS})

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
    src/DynamicDescriptors.cpp
    src/AssetLoader.cpp
    src/TextRenderer.cpp
    src/DMalloc.cpp
    # Atlas/Atlas.cpp
)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/Atlas)
include_directories(${CMAKE_SOURCE_DIR}/UIX)
include_directories(${CMAKE_SOURCE_DIR}/build)

add_subdirectory(${CMAKE_SOURCE_DIR}/Atlas)
add_subdirectory(${CMAKE_SOURCE_DIR}/VulkanMemoryAllocator)

set(ATLAS_SOURCES
    # Atlas/Atlas.cpp
    Atlas/main.cpp
    src/AssetLoader.cpp
)

set(ATLAS_LIBS
    stb
    vulkan
)

set(LIBS
    vulkan
    SDL3
    stb
    freetype
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