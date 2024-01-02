# Copyright 2015-2024 IQRF Tech s.r.o.
# Copyright 2019-2024 MICRORISC s.r.o.
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

project(IqrfInfo)

if(UNIX)
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
  if(COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++1y -pthread")
  else()
    message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
  endif()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
endif()

if(UNIX)
  FIND_PACKAGE(SQLite3 REQUIRED)
else()
  if(${VCPKG_VER} STREQUAL "2021-01-13-unknownhash")
    message(WARNING "VCPKG_VER: ${VCPKG_VER}")
    FIND_PACKAGE(unofficial-sqlite3 CONFIG REQUIRED)
  else()
    FIND_PACKAGE(SQLite3 REQUIRED)
  endif()
endif()

set(COMPONENT iqrf::IqrfInfo)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IIqrfInfo)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsCacheService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/libraries/sqlite_modern_cpp/hdr)
include_directories(${CMAKE_SOURCE_DIR}/src/DpaParser)

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

if(UNIX)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    sqlite3
  )
else()
  if(${VCPKG_VER} STREQUAL "2021-01-13-unknownhash")
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::sqlite3::sqlite3)
  else()
    target_link_libraries(${PROJECT_NAME} PRIVATE sqlite3)
  endif()
endif()

DeployShapeComponent(${PROJECT_NAME})

if(WIN32)
	install(
		FILES
			"$<TARGET_FILE_DIR:${PROJECT_NAME}>/sqlite3.dll"
		CONFIGURATIONS Debug
		DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	)

	install(
		FILES
			"$<TARGET_FILE_DIR:${PROJECT_NAME}>/sqlite3.dll"
		CONFIGURATIONS Release
		DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	)
endif()