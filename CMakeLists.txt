project(IqrfDaemon)

cmake_minimum_required(VERSION 3.0)

enable_testing()

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

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)

FIND_PACKAGE(shape REQUIRED)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
include(${CMAKE_MODULE_PATH}/ShapeComponentDeclaration.cmake)

add_definitions(-DRAPIDJSON_HAS_STDSTRING)

include_directories(${CMAKE_SOURCE_DIR}/include)


add_subdirectory(libraries/clibspi  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibcdc  EXCLUDE_FROM_ALL)
add_subdirectory(libraries/clibdpa  EXCLUDE_FROM_ALL)

#if (NOT WIN32) 
  set(NO_UART_SYSFS_GPIO true) #don't define duplicit target sysfs_gpio provided already bz clibspi
  add_subdirectory(libraries/clibuart  EXCLUDE_FROM_ALL)
#endif()

add_subdirectory(include)
add_subdirectory(start-IqrfDaemon)
add_subdirectory(Scheduler)
add_subdirectory(IqrfCdc)
add_subdirectory(IqrfSpi)
#if (NOT WIN32) 
  add_subdirectory(IqrfUart)
#endif()
add_subdirectory(IqrfDpa)
add_subdirectory(LegacyApiSupport)
add_subdirectory(MqMessaging)
add_subdirectory(MqttMessaging)
add_subdirectory(WebsocketMessaging)
add_subdirectory(UdpMessaging)
add_subdirectory(IdeCounterpart)
add_subdirectory(JsonSplitter)
add_subdirectory(JsonDpaApiRaw)
add_subdirectory(JsonMngApi)
add_subdirectory(JsonCfgApi)
add_subdirectory(JsCache)
add_subdirectory(JsRenderDuktape)
add_subdirectory(JsonDpaApiIqrfStandard)
add_subdirectory(JsonDpaApiIqrfStdExt)
add_subdirectory(SchedulerMessaging)
add_subdirectory(IqmeshServices/BondNodeLocalService)
add_subdirectory(IqmeshServices/SmartConnectService)
add_subdirectory(IqmeshServices/OtaUploadService)
add_subdirectory(IqmeshServices/ReadTrConfService)
add_subdirectory(IqmeshServices/WriteTrConfService)
add_subdirectory(IqmeshServices/EnumerateDeviceService)
add_subdirectory(NativeUpload)

if(${BUILD_TESTING})
  add_subdirectory(tests/TestScheduler)
  add_subdirectory(tests/TestJsRender)
endif()


set(DEFAULT_DAEMON_VERSION v2.0.0dev)
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo.h.in ${CMAKE_BINARY_DIR}/VersionInfo.h @ONLY)
