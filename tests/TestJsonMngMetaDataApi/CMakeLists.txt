project(TestJsonMngMetaDataApi)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestJsonMngMetaDataApi)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::IMetaDataApi MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationMessaging MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _SRCMETADATA ${CMAKE_CURRENT_SOURCE_DIR}/metaData/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})
source_group("MetaData" FILES ${_SRCMETADATA})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../JsonMngMetaDataApi")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})


if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
endif()

target_link_libraries(${PROJECT_NAME} GTestStaticRunner ${GoogleTest_LIBRARIES})

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})

DeployShapeComponent(${PROJECT_NAME})

DeployShapeConfiguration(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/configuration	configuration
	${CMAKE_SOURCE_DIR}/libraries/iqrf-daemon-api/JsonSchemas configuration/da-ta-dir-share/apiSchemas
	${CMAKE_CURRENT_SOURCE_DIR}/json	configuration/json
	${CMAKE_CURRENT_SOURCE_DIR}/metaData	configuration/metaData
)

DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

DeployShapeConfigurationTest("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})
