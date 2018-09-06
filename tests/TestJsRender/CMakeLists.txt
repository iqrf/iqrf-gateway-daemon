project(TestJsRender)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestJsRender)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
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
file(GLOB_RECURSE _SRCCONFIG ${_CONFIG}/*)
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

target_link_libraries(${PROJECT_NAME} ${GoogleTest_LIBRARIES} GTestStaticRunner)

# do the copying
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CONFIG} "${CMAKE_CURRENT_BINARY_DIR}/configuration"
)

#copy TestJavaScript
set(_CFG_TJS_FROM ${CMAKE_CURRENT_SOURCE_DIR}/TestJavaScript)
set(_CFG_TJS_TO "${CMAKE_CURRENT_BINARY_DIR}/TestJavaScript")
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_TJS_FROM} ${_CFG_TJS_TO}
)

add_test(
	NAME ${PROJECT_NAME}
	COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/iqrfgd2 ${CMAKE_CURRENT_BINARY_DIR}/configuration/config.json
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
