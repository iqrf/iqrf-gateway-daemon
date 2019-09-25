cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project("IQRF console")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(cpp)
add_subdirectory(utility)

add_executable(console console.c)

target_link_libraries(console cpp utility)
