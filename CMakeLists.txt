cmake_minimum_required(VERSION 3.22)

project(pea_z1_gusta)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Packages.cmake)
add_subdirectory(src)
