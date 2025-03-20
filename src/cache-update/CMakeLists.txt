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

project(iqrfgd2-cache-updater)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(LIBZIP REQUIRED)

set(PROJECT_SOURCES
    "update-cache.cpp"
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${LIBZIP_LIBRARY}
    cpr::cpr
    nlohmann_json::nlohmann_json
    ValiJSON::valijson
    pthread
)

add_custom_target(iqrfgd2-cache-update
    COMMAND ${PROJECT_NAME} -p ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Updating IQRF repository cache"
    SOURCES ${PROJECT_SOURCES}
)

list(APPEND CfgList "LinCdc" "LinSpi" "LinUart" "LinDeploy")

foreach(cfg ${CfgList})
    DeployShapeConfiguration(iqrfgd2-${cfg}
        ${CMAKE_CURRENT_SOURCE_DIR}/cache       configuration/iqrfRepoCache/cache
        ${CMAKE_CURRENT_SOURCE_DIR}/schemas     configuration/iqrfRepoCache/schemas
    )
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/serverCheck.json
        DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/runcfg/iqrfgd2-${cfg}/configuration/iqrfRepoCache"
    )
endforeach()
