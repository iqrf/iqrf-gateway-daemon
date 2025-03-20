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

project(DpaParser)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("Header Files" FILES ${_HDRFILES})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
source_group("Source Files" FILES ${_SRCFILES})

include_directories(${clibdpa_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/libraries/clibdpa/Dpa)

#add_custom_target(${PROJECT_NAME} SOURCES ${_SRCFILES})
add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
