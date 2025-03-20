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

project(TestScheduler)

find_package(GoogleTest REQUIRED)

set(COMPONENT iqrf::TestScheduler)
DeclareShapeComponent(${COMPONENT})
AddShapeRequiredInterface(${COMPONENT} iqrf::ISchedulerService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _SRCCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _SRCSCHED ${CMAKE_CURRENT_SOURCE_DIR}/scheduler/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_SRCCONFIG})
source_group("Scheduler" FILES ${_SRCSCHED})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${GoogleTest_INCLUDE_DIR})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG} ${_SRCSCHED})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES} ${_SRCCONFIG} ${_SRCSCHED})
endif()

target_link_libraries(${PROJECT_NAME} GTestStaticRunner ${GoogleTest_LIBRARIES})

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})

DeployShapeComponent(${PROJECT_NAME})

DeployShapeConfiguration(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/configuration	    configuration
	${CMAKE_CURRENT_SOURCE_DIR}/scheduler		    scheduler
    ${CMAKE_CURRENT_SOURCE_DIR}/schedulerSchemas    schedulerSchemas
)

DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

DeployShapeConfigurationTest("${PROJECT_INSTALL_PREFIX}/bin/iqrfgd2" ${PROJECT_NAME})

