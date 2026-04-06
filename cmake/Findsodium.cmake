# Copyright 2015-2026 IQRF Tech s.r.o.
# Copyright 2019-2026 MICRORISC s.r.o.
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

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
pkg_check_modules(PC_libsodium QUIET libsodium)

# Look for the header file
find_path(sodium_INCLUDE_DIR
  NAMES sodium.h
  HINTS ${PC_libsodium_INCLUDEDIR} ${PC_libsodium_INCLUDE_DIRS}
)

set(sodium_VERSION ${PC_libsodium_VERSION})

# Look for the library
find_library(sodium_LIBRARY
  NAMES sodium
  HINTS ${PC_libsodium_LIBDIR} ${PC_libsodium_LIBRARY_DIRS}
)

find_package_handle_standard_args(sodium
  FOUND_VAR sodium_FOUND
  REQUIRED_VARS sodium_LIBRARY sodium_INCLUDE_DIR
  VERSION_VAR sodium_VERSION
)

if(sodium_FOUND)
  set(sodium_LIBRARIES ${sodium_LIBRARY})
  set(sodium_INCLUDE_DIRS ${sodium_INCLUDE_DIR})
  message(STATUS "Found libsodium: ${sodium_LIBRARY}")
  message(STATUS "Found libsodium version: ${sodium_VERSION}")
else()
  set(sodium_LIBRARIES)
  set(sodium_INCLUDE_DIRS)
  message(WARNING "No shared libsodium library found.")
endif()

mark_as_advanced(sodium_INCLUDE_DIRS sodium_LIBRARIES)
