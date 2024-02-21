# Copyright 2015-2025 IQRF Tech s.r.o.
# Copyright 2019-2025 MICRORISC s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(MonitorService)

FIND_PACKAGE(shapeware REQUIRED)

set(COMPONENT iqrf::MonitorService)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IMonitorService)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDb UNREQUIRED SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService UNREQUIRED SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfSensorData UNREQUIRED SINGLE)
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
include_directories(${CMAKE_SOURCE_DIR}/src/DpaParser)

add_definitions(-DRAPIDJSON_HAS_STDSTRING)

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

target_link_libraries(${PROJECT_NAME}
    nlohmann_json::nlohmann_json
	Dpa
)

DeployShapeComponent(${PROJECT_NAME})
