cmake_minimum_required(VERSION 3.16)
project(main)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# The name of the output
set(OUTPUT
    main
)

include_directories(${CMAKE_SOURCE_DIR}/Atlas)

set(PRECOMPILED_HEADERS
    include/stdafx.hpp
    include/vkcb/vkcbstdafx.hpp
    include/cengine.hpp
    include/cinput.hpp
)

set(SOURCES
    src/main.cpp
    src/cengine.cpp
    src/cinput.cpp

    src/engine/ctext/ctext.cpp
    
    src/vkcb/pro.cpp
    src/vkcb/Context.cpp
    src/vkcb/Bootstrap.cpp
    src/vkcb/Renderer.cpp
    src/vkcb/epichelperlib.cpp
    src/vkcb/cobject.cpp
    src/engine/object/cgameobject.cpp
)

set(MSDFGEN_CORE_ONLY OFF)
set(MSDFGEN_BUILD_STANDALONE OFF)
set(MSDFGEN_USE_SKIA OFF)
set(MSDFGEN_USE_VCPKG OFF)

set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_DYNAMIC_RUNTIME OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)

include_directories(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen/msdfgen)
include_directories(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen/msdf-atlas-gen)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen)

set(LIBS
    vulkan
    SDL2
    msdf-atlas-gen
    ${FREETYPE_LIBRARIES}
)

execute_process(
    COMMAND ${CMAKE_SOURCE_DIR}/Shaders/compile.sh ${CMAKE_BINARY_DIR}/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Shaders
)

add_executable(${OUTPUT} ${SOURCES})
target_link_options(${OUTPUT} PRIVATE -g -Wall -fopenmp)
target_compile_options(${OUTPUT} PRIVATE -g -Wall -fopenmp)
target_link_libraries(${OUTPUT} ${LIBS})

add_executable(shader_manager src/vkcb/cshader_manager.c)

target_precompile_headers(${OUTPUT} PRIVATE ${PRECOMPILED_HEADERS})