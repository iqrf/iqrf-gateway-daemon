project(OffGridCoreMcu)

find_package(shapeware REQUIRED)
#find_package(iqrf-gateway-daemon REQUIRED)

set(COMPONENT iqrf::OffGridCoreMcu)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IOffGridCoreMcu)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfChannelService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ICommandService UNREQUIRED SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${iqrf-gateway-daemon_INCLUDE_DIRS})

add_definitions(-DRAPIDJSON_HAS_STDSTRING)

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

#set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${ADAPTORS_FOLDER})

DeployShapeComponent(${PROJECT_NAME})
