project(BaseService)

set(COMPONENT iqrf::BaseService)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IBaseService)
AddShapeRequiredInterface(${COMPONENT} iqrf::IMessagingSplitterService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ISchedulerService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()
