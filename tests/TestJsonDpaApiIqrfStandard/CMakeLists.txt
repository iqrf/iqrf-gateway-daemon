project(TestJsonDpaApiIqrfStandard)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestJsonDpaApiIqrfStandard)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationIqrfChannel MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationMessaging MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

if(WIN32)
  set(_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configurationWin)
else()
  set(_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configurationLin)
endif()

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

target_link_libraries(${PROJECT_NAME} ${GoogleTest_LIBRARIES} GTestStaticRunner)

# do the copying
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CONFIG} "${CMAKE_CURRENT_BINARY_DIR}/configuration"
)

#copy api schemas
set(_JSON_SCHEMES_FROM ${CMAKE_SOURCE_DIR}/libraries/iqrf-daemon-api/JsonSchemas)
set(_JSON_SCHEMES_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/apiSchemas)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JSON_SCHEMES_FROM} ${_JSON_SCHEMES_TO}
)

#copy javaScript/DaemonWrapper.js
set(_JAVA_SCRIPT_FROM ${CMAKE_SOURCE_DIR}/start-IqrfDaemon/JavaScript)
set(_JAVA_SCRIPT_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/javaScript)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JAVA_SCRIPT_FROM} ${_JAVA_SCRIPT_TO}
)

#copy scheduler/Task.json
set(_CFG_SCHEDULER_FROM ${CMAKE_SOURCE_DIR}/start-IqrfDaemon/scheduler)
set(_CFG_SCHEDULER_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/scheduler)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_SCHEDULER_FROM} ${_CFG_SCHEDULER_TO}
)

#copy offline repository
set(_IQRF_REPO_FROM ${CMAKE_SOURCE_DIR}/start-IqrfDaemon/iqrfRepoCache)
set(_IQRF_REPO_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/iqrfRepoCache)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_REPO_FROM} ${_IQRF_REPO_TO}
)

add_test(
	NAME TestJsRender
	COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/iqrfgd2 ${CMAKE_CURRENT_BINARY_DIR}/configuration/config.json
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
