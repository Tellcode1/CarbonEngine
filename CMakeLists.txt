cmake_minimum_required(VERSION 3.10)
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
    src/cengine.c
    src/cengineinit.c
    
    src/ctext.c
    src/cshadermanager.c
    
    src/lunaPipeline.c
    src/lunaGFX.c
    src/lunaVK.c

    src/containersunity.c

    src/lunaImageIO.c
    src/sprite.c
)

add_library(spirv-reflect STATIC ${CMAKE_SOURCE_DIR}/external/SPIRV-Reflect/spirv_reflect.c)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/volk)

add_executable(${OUTPUT} ${SOURCES})

add_executable(shadermanager src/cshadermanager.c)
target_compile_definitions(shadermanager PRIVATE CSM_EXECUTABLE=1)
target_compile_options(shadermanager PRIVATE -O3 -g)

set(LIBS
    m
    volk
    SDL2
    freetype
    jpeg
    png
    spirv-reflect
)

target_link_options(${OUTPUT} PRIVATE -g -Wall -Wextra)
target_compile_options(${OUTPUT} PRIVATE -g -Wall -Wextra)
target_compile_definitions(${OUTPUT} PRIVATE CSM_EXECUTABLE=0) # avoid making multiple main's 
target_link_libraries(${OUTPUT} ${LIBS})