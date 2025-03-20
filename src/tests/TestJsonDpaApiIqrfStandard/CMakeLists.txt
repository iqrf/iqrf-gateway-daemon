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

project(TestJsonDpaApiIqrfStandard)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestJsonDpaApiIqrfStandard)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationIqrfChannel MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ITestSimulationMessaging MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG})
endif()

target_link_libraries(${PROJECT_NAME} GTestStaticRunner ${GoogleTest_LIBRARIES})

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})

DeployShapeComponent(${PROJECT_NAME})

DeployShapeConfiguration(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/configuration					configuration
	${CMAKE_SOURCE_DIR}/api 									configuration/apiSchemas
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/javaScript 		configuration/javaScript
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/schedulerSchemas 	configuration/schedulerSchemas
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/DB/init 			configuration//DB/init
	${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/iqrfRepoCache 		configuration/iqrfRepoCache
)

DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

DeployShapeConfigurationTest("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})
