project(TestJsonDpaApiIqrfStandard)

set(COMPONENT iqrf::TestJsonDpaApiIqrfStandard)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationIqrfChannel MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationMessaging MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(SHAPE_STATIC_LIBS)
	add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
else()
	add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
	GTestStaticRunner
	gtest
	gtest_main
	gmock
	gmock_main
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})

DeployShapeComponent(${PROJECT_NAME})

DeployShapeConfiguration(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/configuration										configuration
	${CMAKE_SOURCE_DIR}/api 																		configuration/apiSchemas
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/javaScript 				configuration/javaScript
	${CMAKE_SOURCE_DIR}/src/cache-update/schemas								configuration/cacheSchemas
	${CMAKE_SOURCE_DIR}/src/cache-update/cache									configuration/iqrfRepoCache
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/schedulerSchemas 	configuration/schedulerSchemas
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/DB/migrations 			configuration/DB/migrations
)

DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

DeployShapeConfigurationTest("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})
