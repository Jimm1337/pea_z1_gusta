file(GLOB_RECURSE SRC_FILES ./*.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)

target_compile_definitions(${PROJECT_NAME} PRIVATE DATA_DIR="${CMAKE_SOURCE_DIR}/data/")
