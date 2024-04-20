cmake_minimum_required(VERSION 3.15)
project(Atlas)

set(Atlas_SOURCES
    main.cpp
    ../src/AssetLoader.cpp
)

set(Atlas_LIBS
    stb
    vulkan
)

add_executable(Atlas ${Atlas_SOURCES})

set_target_properties(Atlas
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Atlas/build"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Atlas/build"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Atlas/build"
)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(Atlas ${Atlas_LIBS})