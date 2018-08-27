project(TestScheduler)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestScheduler)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::ISchedulerService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _SRCSCHED ${CMAKE_CURRENT_SOURCE_DIR}/scheduler/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG} ${_SRCSCHED})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG} ${_SRCSCHED})
endif()

target_link_libraries(${PROJECT_NAME} ${GoogleTest_LIBRARIES} GTestStaticRunner)

# do the copying
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration "${CMAKE_CURRENT_BINARY_DIR}/configuration"
)

#copy scheduler/Task.json
set(_CFG_SCHEDULER_FROM ${CMAKE_CURRENT_SOURCE_DIR}/scheduler)
set(_CFG_SCHEDULER_TO "${CMAKE_CURRENT_BINARY_DIR}/scheduler")
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_SCHEDULER_FROM} ${_CFG_SCHEDULER_TO}
)

add_test(
	NAME TestScheduler
	COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/iqrfgd2 ${CMAKE_CURRENT_BINARY_DIR}/configuration/config.json
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
