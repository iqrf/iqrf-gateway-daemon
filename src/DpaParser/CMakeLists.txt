project(DpaParser)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("Header Files" FILES ${_HDRFILES})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/libraries/clibdpa/Dpa)

#add_custom_target(${PROJECT_NAME} SOURCES ${_SRCFILES})
add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
