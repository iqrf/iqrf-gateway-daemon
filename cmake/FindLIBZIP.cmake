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

find_path(LIBZIP_INCLUDE_DIR NAMES zip.h)
mark_as_advanced(LIBZIP_INCLUDE_DIR)

find_library(LIBZIP_LIBRARY NAMES zip)
mark_as_advanced(LIBZIP_LIBRARY)

get_filename_component(_libzip_libdir ${LIBZIP_LIBRARY} DIRECTORY)
find_file(_libzip_pkgcfg libzip.pc
    HINTS ${_libzip_libdir} ${LIBZIP_INCLUDE_DIR}/..
    PATH_SUFFIXES pkgconfig lib/pkgconfig
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LIBZIP
    REQUIRED_VARS
        LIBZIP_LIBRARY
        LIBZIP_INCLUDE_DIR
        _libzip_pkgcfg
)

if (LIBZIP_FOUND)
    if (NOT TARGET libzip::libzip)
        add_library(libzip::libzip UNKNOWN IMPORTED)
        set_target_properties(libzip::libzip
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES ${LIBZIP_INCLUDE_DIR}
                INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
                IMPORTED_LOCATION ${LIBZIP_LIBRARY}
        )
        # (Ab)use the (always) installed pkgconfig file to check if BZip2 is required
        file(STRINGS ${_libzip_pkgcfg} _have_bzip2 REGEX Libs)
        if(_have_bzip2 MATCHES "-lbz2")
            find_package(BZip2 REQUIRED)
            set_property(TARGET libzip::libzip APPEND PROPERTY INTERFACE_LINK_LIBRARIES BZip2::BZip2)
        endif()
    endif()
endif()
