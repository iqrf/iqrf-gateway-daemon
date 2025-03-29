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

set(PROJECT_VERSION "2.99.3-alpha")
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
add_subdirectory(libraries/sqlite_orm EXCLUDE_FROM_ALL)
add_subdirectory(libraries/sqlitec++ EXCLUDE_FROM_ALL)

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
add_subdirectory(src/JsonDbApi)
add_subdirectory(src/JsCache)
add_subdirectory(src/JsRenderDuktape)
add_subdirectory(src/JsonDpaApiIqrfStandard)
add_subdirectory(src/JsonDpaApiIqrfStdExt)
add_subdirectory(src/SchedulerMessaging)
add_subdirectory(src/IqrfDb)
add_subdirectory(src/IqrfBackup)
add_subdirectory(src/IqrfRestore)
add_subdirectory(src/IqrfSensorData)
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
	#add_subdirectory(src/tests)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/VersionInfo.h.in ${CMAKE_BINARY_DIR}/VersionInfo.h @ONLY)
