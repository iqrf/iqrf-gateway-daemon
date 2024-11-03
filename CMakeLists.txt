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

cmake_minimum_required(VERSION 3.18)

project(iqrf-gateway-daemon)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

set(PROJECT_VERSION "2.6.0-rc1")
if (DEFINED PIPELINE_ID AND NOT PIPELINE_ID STREQUAL "")
	set(PROJECT_VERSION "${PROJECT_VERSION}~${PIPELINE_ID}")
endif ()
set(APP_VERSION ${PROJECT_VERSION})
message(STATUS "Version = v" ${APP_VERSION})

include(${PROJECT_SOURCE_DIR}/cmake/ccache.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/hardening.cmake)

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)

find_package(shape REQUIRED)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
include(${CMAKE_MODULE_PATH}/ShapeComponentDeclaration.cmake)

set(PROJECT_INSTALL_PREFIX ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${shape_DEPLOY})

add_definitions(
  -DRAPIDJSON_HAS_STDSTRING
  -D_CRT_SECURE_NO_WARNINGS
)
set(IHP_BUILD_STATIC ON)

find_package(CURL REQUIRED)

add_subdirectory(libraries/clibspi  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibcdc  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibdpa  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibuart EXCLUDE_FROM_ALL)
add_subdirectory(libraries/iqrf-header-parser EXCLUDE_FROM_ALL)
add_subdirectory(libraries/croncpp EXCLUDE_FROM_ALL)
add_subdirectory(libraries/date EXCLUDE_FROM_ALL)
add_subdirectory(libraries/cpr EXCLUDE_FROM_ALL)
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(libraries/nlohmann_json EXCLUDE_FROM_ALL)
set(valijson_BUILD_TESTS OFF)
add_subdirectory(libraries/valijson EXCLUDE_FROM_ALL)

#add_subdirectory(src)

enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

add_subdirectory(src/include)
add_subdirectory(src/start-IqrfDaemon)
add_subdirectory(src/cache-update)
add_subdirectory(src/Scheduler)
add_subdirectory(src/IqrfCdc)
if (WIN32)
 add_subdirectory(src/IqrfUartWin)
endif()
add_subdirectory(src/IqrfSpi)
add_subdirectory(src/IqrfTcp)
#if (NOT WIN32)
  add_subdirectory(src/IqrfUart)
#endif()
add_subdirectory(src/IqrfDpa)
add_subdirectory(src/MqMessaging)
add_subdirectory(src/MqttMessaging)
add_subdirectory(src/BufferedMqttMessaging)
add_subdirectory(src/WebsocketMessaging)
add_subdirectory(src/UdpMessaging)
add_subdirectory(src/IdeCounterpart)
add_subdirectory(src/JsonSplitter)
add_subdirectory(src/JsonDpaApiRaw)
add_subdirectory(src/JsonMngApi)
add_subdirectory(src/JsonCfgApi)
add_subdirectory(src/JsCache)
add_subdirectory(src/JsRenderDuktape)
add_subdirectory(src/JsonDpaApiIqrfStandard)
add_subdirectory(src/JsonDpaApiIqrfStdExt)
add_subdirectory(src/JsonIqrfInfoApi)
add_subdirectory(src/SchedulerMessaging)
add_subdirectory(src/IqrfInfo)
add_subdirectory(src/IqrfBackup)
add_subdirectory(src/IqrfRestore)
add_subdirectory(src/MonitorService)
add_subdirectory(src/DpaParser)
if (NOT WIN32)
  add_subdirectory(src/SyslogLogger)
endif()

# IDE folder for IQMesh services
set(IQMESH_SERVICES_FOLDER "iqmesh")
add_subdirectory(src/IqmeshServices/BondNodeLocalService)
add_subdirectory(src/IqmeshServices/SmartConnectService)
add_subdirectory(src/IqmeshServices/OtaUploadService)
add_subdirectory(src/IqmeshServices/ReadTrConfService)
add_subdirectory(src/IqmeshServices/WriteTrConfService)
add_subdirectory(src/IqmeshServices/EnumerateDeviceService)
add_subdirectory(src/IqmeshServices/AutonetworkService)
add_subdirectory(src/IqmeshServices/BackupService)
add_subdirectory(src/IqmeshServices/RestoreService)
add_subdirectory(src/IqmeshServices/RemoveBondService)
add_subdirectory(src/IqmeshServices/PingService)
add_subdirectory(src/IqmeshServices/RestartService)
add_subdirectory(src/IqmeshServices/Maintenance)
add_subdirectory(src/IqmeshServices/DpaParams)

if(${BUILD_TESTING})
  add_subdirectory(src/tests)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/VersionInfo.h.in ${CMAKE_BINARY_DIR}/VersionInfo.h @ONLY)
