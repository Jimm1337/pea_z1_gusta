cmake_minimum_required(VERSION 3.22)

project(pea_z1_gusta)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MT - Statically link the runtime library
add_compile_options(/MT)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(/Od /Zi)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
#    O2 - General optimization
#    Ob3 - Inline function expansion (aggresive)
#    Oi - Intrinsic functions
#    Ot - Favor fast code instead of binary size
#    Oy - Frame pointer omission
#    GL - Whole program optimization
#    arch:SSE4.2 - Enable SSE4.2 instructions
#    LTCG - Link-time code generation
    add_compile_options(/O2 /Ob3 /Oi /Ot /Oy /GL /arch:SSE4.2)
    add_link_options(/LTCG)
else ()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE} (use Debug or Release)")
endif ()

include(Packages.cmake)
add_subdirectory(external)
add_subdirectory(src)
