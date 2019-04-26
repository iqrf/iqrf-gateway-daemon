cmake_minimum_required(VERSION 3.0)

project(iqrf-gateway-daemon)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake|MSBuild)")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
  CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -pthread")
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++0x -pthread")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
  endif()
endif()

############# warnings
#set(CPPFLAGS "-Wdate-time -D_FORTIFY_SOURCE=2")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -Wall -Wextra ${CPPFLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -Wall -Wextra ${CPPFLAGS}")
#############

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)

find_package(shape REQUIRED)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
include(${CMAKE_MODULE_PATH}/ShapeComponentDeclaration.cmake)

set(PROJECT_INSTALL_PREFIX ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${shape_DEPLOY})

add_definitions(
  -DRAPIDJSON_HAS_STDSTRING
  -D_CRT_SECURE_NO_WARNINGS
)

set(DEFAULT_DAEMON_VERSION v2.1.0dev)
string(TIMESTAMP DEFAULT_BUILD_TIMESTAMP)

if(DEFINED DAEMON_VERSION)
  message(STATUS "DAEMON_VERSION = " ${DAEMON_VERSION})
else()
  message(STATUS "Set to default DAEMON_VERSION = " ${DEFAULT_DAEMON_VERSION})
  set(DAEMON_VERSION ${DEFAULT_DAEMON_VERSION})
endif()

if(DEFINED BUILD_TIMESTAMP)
  #message(STATUS "BUILD_TIMESTAMP = " ${BUILD_TIMESTAMP})
else()
  message(STATUS "Set to default BUILD_TIMESTAMP = " ${DEFAULT_BUILD_TIMESTAMP})
  set(BUILD_TIMESTAMP ${DEFAULT_BUILD_TIMESTAMP})
endif()

add_subdirectory(libraries/clibspi  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibcdc  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibdpa  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibuart EXCLUDE_FROM_ALL)

add_subdirectory(src)