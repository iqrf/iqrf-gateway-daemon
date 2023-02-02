# Copyright 2015-2023 IQRF Tech s.r.o.
# Copyright 2019-2023 MICRORISC s.r.o.
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