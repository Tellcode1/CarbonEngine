cmake_minimum_required(VERSION 3.10)
project(main)

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
    src/main.cpp
    src/cengine.cpp
    src/cengineinit.cpp
    src/cinput.c
    
    src/ctext.cpp
    src/cshadermanager.c
    
    src/cvk.cpp
    src/cgfx.cpp
    src/vkhelper.cpp

    src/containers/containersunity.c
    src/containers/cbinarytree.c

    src/cimageload.c
)

set(MSDFGEN_CORE_ONLY OFF)
set(MSDFGEN_BUILD_STANDALONE OFF)
set(MSDFGEN_USE_SKIA OFF)
set(MSDFGEN_USE_VCPKG OFF)

set(MSDF_ATLAS_USE_VCPKG OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_DYNAMIC_RUNTIME OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)

add_executable(${OUTPUT} ${SOURCES})

target_include_directories(${OUTPUT} PUBLIC ${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen/msdfgen)
target_include_directories(${OUTPUT} PUBLIC ${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen/msdf-atlas-gen)
target_include_directories(${OUTPUT} PUBLIC ${CMAKE_SOURCE_DIR}/external/SPIRV-Reflect)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/msdf-atlas-gen)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/volk)

add_executable(shadermanager src/cshadermanager.c)
target_compile_definitions(shadermanager PRIVATE CSM_EXECUTABLE=1)
target_compile_options(shadermanager PRIVATE -O3 -g)

set(LIBS
    volk
    SDL2
    msdf-atlas-gen
    freetype
    jpeg
)

target_link_options(${OUTPUT} PRIVATE -g -Wall -fopenmp)
target_compile_options(${OUTPUT} PRIVATE -g -Wall -fopenmp)
target_compile_definitions(${OUTPUT} PRIVATE CSM_EXECUTABLE=0) # avoid making multiple main's 
target_link_libraries(${OUTPUT} ${LIBS})