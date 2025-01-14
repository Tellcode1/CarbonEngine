cmake_minimum_required(VERSION 3.16)
project(impact)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(impact STATIC src/impact.c)