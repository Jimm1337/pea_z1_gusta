file (GLOB_RECURSE INIH_SOURCE_C ${CMAKE_SOURCE_DIR}/external/inih/src/*.c)
file (GLOB_RECURSE INIH_SOURCE_CPP ${CMAKE_SOURCE_DIR}/external/inih/src/*.cpp)

add_library(inih STATIC ${INIH_SOURCE_C} ${INIH_SOURCE_CPP})

target_include_directories(inih PUBLIC ${CMAKE_SOURCE_DIR}/external/inih/inc)
