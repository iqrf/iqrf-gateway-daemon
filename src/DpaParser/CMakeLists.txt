project(DpaParser)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("Header Files" FILES ${_HDRFILES})

add_custom_target(${PROJECT_NAME} SOURCES ${_HDRFILES})
