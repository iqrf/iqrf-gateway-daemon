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
