cmake_minimum_required(VERSION 3.16)
project(main)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

# The name of the output
set(OUTPUT
    main
)

set(SOURCES
    src/main.c
    src/engine.c
    src/preprocessors.c
    src/containers.c
    src/lunaVK.c
)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/box2d)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/volk)

add_executable(${OUTPUT} ${SOURCES})

# LC was lunaCore btw
# I forgot what it meant after an hour so that's why I'm replacing it

# core is the common library that every library should link with
add_library(core STATIC src/common.c)
target_compile_options(core PRIVATE -O3)
target_link_libraries(core jpeg png z)

add_executable(shadermanager src/preprocessors.c)
target_compile_definitions(shadermanager PRIVATE CSM=1 CSM_EXECUTABLE=1)
target_link_libraries(shadermanager m core)
target_compile_options(shadermanager PRIVATE -O3 --fast-math)

add_executable(fontc src/preprocessors.c)
target_compile_definitions(fontc PRIVATE FONTC=1 FONTC_EXECUTABLE=1)
target_link_libraries(fontc freetype m core)
target_compile_options(fontc PRIVATE -O3 -g)

set(LIBS
    core
    volk
    m
    SDL2
    freetype
    box2d
)

target_precompile_headers(${OUTPUT} PUBLIC ${CMAKE_SOURCE_DIR}/include/common/stdafx.h)
target_link_options(${OUTPUT} PRIVATE -g -Wall -Wextra)
target_compile_options(${OUTPUT} PRIVATE -g -Wall -Wextra)
target_compile_definitions(${OUTPUT} PRIVATE CSM FONTC FONTC_EXECUTABLE=0 CSM_EXECUTABLE=0) # avoid making multiple main's 
target_link_libraries(${OUTPUT} ${LIBS})