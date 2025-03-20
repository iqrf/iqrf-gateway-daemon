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

project(JsCache)

find_package(Boost REQUIRED COMPONENTS filesystem )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(LIBZIP REQUIRED)
find_package(shapeware REQUIRED)

set(COMPONENT iqrf::JsCache)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IJsCacheService)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ISchedulerService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::IRestApiService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/DpaParser)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

target_link_libraries(${PROJECT_NAME}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${LIBZIP_LIBRARY}
  nlohmann_json::nlohmann_json
)

DeployShapeComponent(${PROJECT_NAME})

if(WIN32)
  if(${VCPKG_VER} STREQUAL "2021-01-13-unknownhash")
	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_filesystem-vc141-mt-gd-x64-1_75.dll"
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/zip.dll"
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/bz2d.dll"
		  CONFIGURATIONS Debug
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )

	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_filesystem-vc141-mt-x64-1_75.dll"
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/zip.dll"
		  CONFIGURATIONS Release
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )
  else()
	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_filesystem-vc141-mt-gd-x64-1_73.dll"
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/zip.dll"
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/bz2d.dll"
		  CONFIGURATIONS Debug
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )

	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_filesystem-vc141-mt-x64-1_73.dll"
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/zip.dll"
		  CONFIGURATIONS Release
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )
  endif()
endif()
