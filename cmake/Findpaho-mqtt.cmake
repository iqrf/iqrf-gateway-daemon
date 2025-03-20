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

# - Find paho-mqtt
# Find the paho-mqtt includes and library from vcpkg
# It's temporary solution on Win, written issue:
# https://github.com/Microsoft/vcpkg/issues/3175
#  paho-mqtt_INCLUDE_DIR - Where to find paho-mqtt includes
#  paho-mqtt_LIBRARIES   - List of libraries when using paho-mqtt
#  paho-mqtt_FOUND       - True if paho-mqtt was found

# To find and use catch
find_path(paho-mqtt_INCLUDE_DIR MQTTAsync.h
  PATHS
  c:/devel/vcpkg/installed/x64-windows/include/paho-mqtt
  DOC "paho-mqtt - Headers"
)

include_directories(${paho-mqtt_INCLUDE_DIR})

SET(paho-mqtt_NAMES paho-mqtt3as.lib paho-mqtt3cs.lib)

FIND_LIBRARY(paho-mqtt_LIBRARY NAMES ${paho-mqtt_NAMES}
  PATHS
  c:/devel/vcpkg/installed/x64-windows/lib
  PATH_SUFFIXES lib lib64
  DOC "paho-mqtt - Library"
)

INCLUDE(FindPackageHandleStandardArgs)

SET(paho-mqtt_LIBRARIES ${paho-mqtt_LIBRARY})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(paho-mqtt DEFAULT_MSG paho-mqtt_LIBRARY paho-mqtt_INCLUDE_DIR)

MARK_AS_ADVANCED(paho-mqtt_LIBRARY paho-mqtt_INCLUDE_DIR)

IF(paho-mqtt_FOUND)
  SET(paho-mqtt_INCLUDE_DIRS ${paho-mqtt_INCLUDE_DIR})
ENDIF(paho-mqtt_FOUND)
