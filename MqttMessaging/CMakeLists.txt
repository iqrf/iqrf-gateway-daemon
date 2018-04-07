project(MqttMessaging)

if (WIN32) 
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
FIND_PACKAGE(paho-mqtt REQUIRED)
else()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
FIND_PACKAGE(paho REQUIRED)
message(STATUS "Found paho: ${paho_FOUND}")
message(STATUS "PAHO_INCLUDE_DIRS: ${PAHO_INCLUDE_DIRS}")
endif()

set(COMPONENT iqrf::MqttMessaging)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IMessagingService)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PAHO_INCLUDE_DIRS})

if (WIN32) 
	link_directories(${PAHO_LIBRARY_DIRS})
endif()

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

if (WIN32) 
target_link_libraries(${PROJECT_NAME}
	${paho-mqtt_LIBRARY}
)
else()
target_link_libraries(${PROJECT_NAME}
	paho-mqtt3a
)
endif()
