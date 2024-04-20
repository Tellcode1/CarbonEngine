cmake_minimum_required(VERSION 3.15)
project(UIX)
project(test)

set(LIBRARY_OUTPUT_TYPE
    STATIC # Use Static or Shared
)

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

set(UIX_Sources
    src/UIX.cpp
)

set(UIX_Libs
    vulkan
    stb
    freetype
)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(UIX ${LIBRARY_OUTPUT_TYPE} ${UIX_Sources})
target_link_libraries(UIX ${UIX_Libs})

add_executable(test src/main.cpp)
target_link_libraries(test UIX ${UIX_Libs})

target_precompile_headers(UIX PRIVATE include/stdafx.hpp)