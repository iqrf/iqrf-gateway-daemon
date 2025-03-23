# IQRF Gateway Daemon
# Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

project(IqrfSensorData)

set(CMAKE_CXX_STANDARD 20)
set(COMPONENT iqrf::IqrfSensorData)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IIqrfSensorData)
AddShapeRequiredInterface(${COMPONENT} shape::IConfigurationService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDb MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsCacheService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IMessagingSplitterService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _DBENTITIES ${CMAKE_SOURCE_DIR}/src/IqrfDb/Entities/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/src/DpaParser)
include_directories(${CMAKE_SOURCE_DIR}/libraries/tinyorm)

add_definitions(-DRAPIDJSON_HAS_STDSTRING)
find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(TinyOrm REQUIRED)

if(SHAPE_STATIC_LIBS)
	add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_DBENTITIES})
else()
	add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_DBENTITIES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
	nlohmann_json::nlohmann_json
	Qt${QT_VERSION_MAJOR}::Core
	TinyOrm::TinyOrm
)

DeployShapeComponent(${PROJECT_NAME})
