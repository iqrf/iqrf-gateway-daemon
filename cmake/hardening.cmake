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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else ()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror=format-security")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=format-security")
endif ()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 3.6)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
	endif()
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 8.0)
		if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i[3456]86|x86")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcf-protection=full")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcf-protection=full")
		endif()
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|^armv8*" OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^arm*")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-clash-protection")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-clash-protection")
		endif()
	endif()
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.9)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.8)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
	endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	if(NOT (CMAKE_C_FLAGS MATCHES ".*-D_FORTIFY_SOURCE=3.*" OR CMAKE_C_FLAGS MATCHES ".*-D_FORTIFY_SOURCE=2.*"))
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2")
	endif ()
	if(NOT (CMAKE_CXX_FLAGS MATCHES ".*-D_FORTIFY_SOURCE=3.*" OR CMAKE_CXX_FLAGS MATCHES ".*-D_FORTIFY_SOURCE=2.*"))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2")
	endif ()
	if(${CMAKE_VERSION} VERSION_LESS 3.13)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now")
	else()
		add_link_options(-Wl,-z,relro -Wl,-z,now)
	endif()
endif()
