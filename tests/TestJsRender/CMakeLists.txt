project(TestJsRender)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestJsRender)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _SRCTJS ${CMAKE_CURRENT_SOURCE_DIR}/TestJavaScript/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})
source_group("Java Script" FILES ${_SRCTJS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG} ${_SRCTJS})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG} ${_SRCTJS})
endif()

target_link_libraries(${PROJECT_NAME} GTestStaticRunner ${GoogleTest_LIBRARIES})

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})

DeployShapeComponent(${PROJECT_NAME})

DeployShapeConfiguration(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/configuration	configuration
	${CMAKE_CURRENT_SOURCE_DIR}/TestJavaScript	TestJavaScript
)

DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

DeployShapeConfigurationTest("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})
