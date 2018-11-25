project(TestJsCache)

find_package(shapeware REQUIRED)
find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestJsCache)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsCacheService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITestSimulationIRestApiService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::IConfigurationService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

if(WIN32)
  set(_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configurationWin)
else()
  set(_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configurationLin)
endif()

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${_CONFIG}/* ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})


if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
endif()

target_link_libraries(${PROJECT_NAME} GTestStaticRunner ${GoogleTest_LIBRARIES})

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})

# system dependant configurationLin/Win
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CONFIG} "${CMAKE_CURRENT_BINARY_DIR}/configuration"
)

# configuration
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration "${CMAKE_CURRENT_BINARY_DIR}/configuration"
)

# javaScript
set(_JAVA_SCRIPT_FROM ${CMAKE_SOURCE_DIR}/start-IqrfDaemon/javaScript)
set(_JAVA_SCRIPT_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/javaScript)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JAVA_SCRIPT_FROM} ${_JAVA_SCRIPT_TO}
)

# configuration JSON schemas
set(_CFG_SCHEMA_FROM ${CMAKE_SOURCE_DIR}/start-IqrfDaemon/cfgSchemas)
set(_CFG_SCHEMA_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/cfgSchemas)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_SCHEMA_FROM} ${_CFG_SCHEMA_TO}
)

# scheduler/Task.json
set(_CFG_SCHEDULER_FROM ${CMAKE_SOURCE_DIR}/start-IqrfDaemon/scheduler)
set(_CFG_SCHEDULER_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/scheduler)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_SCHEDULER_FROM} ${_CFG_SCHEDULER_TO}
)

#copy offline repository 1
set(_IQRF_REPO_FROM ${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache0)
set(_IQRF_REPO_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/testRepoCache/iqrfRepoCache1)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_REPO_FROM} ${_IQRF_REPO_TO}
)

#copy offline repository 2
set(_IQRF_REPO_FROM ${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache0)
set(_IQRF_REPO_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/testRepoCache/iqrfRepoCache2)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_REPO_FROM} ${_IQRF_REPO_TO}
)

#copy test resources 1
set(_IQRF_TESTRES_FROM ${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache0)
set(_IQRF_TESTRES_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/testResources/iqrfRepoResource1/api)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_TESTRES_FROM} ${_IQRF_TESTRES_TO}
)

#copy test resources 2
set(_IQRF_TESTRES_FROM ${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache0)
set(_IQRF_TESTRES_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/testResources/iqrfRepoResource2/api)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_TESTRES_FROM} ${_IQRF_TESTRES_TO}
)

#copy test resources 2
set(_IQRF_TESTRES_FROM ${CMAKE_CURRENT_SOURCE_DIR}/testData)
set(_IQRF_TESTRES_TO ${CMAKE_CURRENT_BINARY_DIR}/configuration/testData)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_TESTRES_FROM} ${_IQRF_TESTRES_TO}
)

add_test(
	NAME ${PROJECT_NAME}
	COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/iqrfgd2 ${CMAKE_CURRENT_BINARY_DIR}/configuration/config.json
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
