cmake_minimum_required(VERSION 3.22)

project(pea_z1_gusta)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(/MT)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(/Od /Zi)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(/O2 /Ob3 /Oi /arch:SSE4.2)
else ()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE} (use Debug or Release)")
endif ()

include(Packages.cmake)
add_subdirectory(external)
add_subdirectory(src)
