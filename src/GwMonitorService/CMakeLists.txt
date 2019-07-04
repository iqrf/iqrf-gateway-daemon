project(GwMonitorService)

FIND_PACKAGE(shapeware REQUIRED)

set(COMPONENT iqrf::GwMonitorService)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IGwMonitorService)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService UNREQUIRED SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IMessagingSplitterService UNREQUIRED SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IUdpConnectorService UNREQUIRED SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::IWebsocketService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${cutils_INCLUDE_DIRS})


add_definitions(-DRAPIDJSON_HAS_STDSTRING)

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

target_link_libraries(${PROJECT_NAME}
	Dpa
)

DeployShapeComponent(${PROJECT_NAME})
