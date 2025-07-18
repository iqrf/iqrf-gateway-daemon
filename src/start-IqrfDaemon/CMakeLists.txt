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

project (iqrfgd2)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _JAVA_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/javaScript/*)

find_package(Boost REQUIRED COMPONENTS program_options)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_CONFIG})
source_group("Configuration" FILES ${_CONFIG_WIN_CDC})
source_group("javaScript" FILES ${_JAVA_SCRIPT})

if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Boost libraries not found.")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG} ${_JAVA_SCRIPT})

add_dependencies(${PROJECT_NAME} iqrfgd2-cache-update)

target_link_libraries(${PROJECT_NAME}
  launcher
  dl
  pthread
  ${Boost_LIBRARIES}
)

DeployShapeComponent(${PROJECT_NAME})

list(APPEND CfgList "LinCdc" "LinSpi" "LinUart" "LinDeploy")

foreach(cfg ${CfgList})
  DeployShapeConfiguration(${PROJECT_NAME}-${cfg}
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration           configuration
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration-${cfg}    configuration
    ${CMAKE_SOURCE_DIR}/api                             configuration/apiSchemas
    ${CMAKE_SOURCE_DIR}/src/cache-update/schemas        configuration/cacheSchemas
    ${CMAKE_CURRENT_SOURCE_DIR}/schedulerSchemas        configuration/schedulerSchemas
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/migrations           configuration/DB/migrations
    ${CMAKE_CURRENT_SOURCE_DIR}/javaScript              configuration/javaScript
    ${CMAKE_CURRENT_SOURCE_DIR}/cfgSchemas              configuration/cfgSchemas
    ${CMAKE_CURRENT_SOURCE_DIR}/systemd                 configuration/systemd
  )
  DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/${PROJECT_NAME}" "${PROJECT_NAME}-${cfg}")
endforeach()
