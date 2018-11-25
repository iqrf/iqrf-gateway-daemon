project (iqrfgd2)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _CONFIG_WIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationWin/*)
file(GLOB_RECURSE _CONFIG_LIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationLin/*)
file(GLOB_RECURSE _JAVA_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/javaScript/*)
file(GLOB_RECURSE _JS_CACHE ${CMAKE_CURRENT_SOURCE_DIR}/jscache/*)
file(GLOB_RECURSE _SYSTEMD ${CMAKE_CURRENT_SOURCE_DIR}/systemd/*)

if(WIN32)
set(_CONFIG_SRC ${_CONFIG_WIN} ${_CONFIG} )
else()
set(_CONFIG_SRC ${_CONFIG_LIN} ${_CONFIG} )
endif()

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_CONFIG_SRC})
source_group("javaScript" FILES ${_JAVA_SCRIPT})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG_SRC} ${_JAVA_SCRIPT})

target_link_libraries(${PROJECT_NAME} 
  launcher
)

if(WIN32)
  set (_CONFIG_DEST ${CMAKE_CURRENT_BINARY_DIR})
  set (_CONFIG_DIR configurationWin)
else()
  set (_CONFIG_DIR configurationLin)
  target_link_libraries(${PROJECT_NAME} 
    dl
    pthread
  )

  set (_CONFIG_DEST $<TARGET_FILE_DIR:${PROJECT_NAME}>)

  INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
  )

endif()

#copy diff configuration
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${_CONFIG_DIR} "${_CONFIG_DEST}/configuration"
)

#copy common
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configuration "${_CONFIG_DEST}/configuration"
)

#copy api schemas
set(_JSON_SCHEMES_FROM ${CMAKE_SOURCE_DIR}/libraries/iqrf-daemon-api/JsonSchemas)
set(_JSON_SCHEMES_TO ${_CONFIG_DEST}/configuration/apiSchemas)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JSON_SCHEMES_FROM} ${_JSON_SCHEMES_TO}
)

#copy javaScript/DaemonWrapper.js
set(_JAVA_SCRIPT_FROM ${CMAKE_CURRENT_SOURCE_DIR}/javaScript)
set(_JAVA_SCRIPT_TO ${_CONFIG_DEST}/configuration/javaScript)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JAVA_SCRIPT_FROM} ${_JAVA_SCRIPT_TO}
)

#copy config schemas
set(_CFG_SCHEMA_FROM ${CMAKE_CURRENT_SOURCE_DIR}/cfgSchemas)
set(_CFG_SCHEMA_TO ${_CONFIG_DEST}/configuration/cfgSchemas)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_SCHEMA_FROM} ${_CFG_SCHEMA_TO}
)

#copy scheduler/Task.json
set(_CFG_SCHEDULER_FROM ${CMAKE_CURRENT_SOURCE_DIR}/scheduler)
set(_CFG_SCHEDULER_TO ${_CONFIG_DEST}/configuration/scheduler)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_CFG_SCHEDULER_FROM} ${_CFG_SCHEDULER_TO}
)

#copy offline repository
set(_IQRF_REPO_FROM ${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache)
set(_IQRF_REPO_TO ${_CONFIG_DEST}/configuration/iqrfRepoCache)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_IQRF_REPO_FROM} ${_IQRF_REPO_TO}
)

#copy systemd service
set(_SYSTEMD_FROM ${CMAKE_CURRENT_SOURCE_DIR}/systemd)
set(_SYSTEMD_TO ${_CONFIG_DEST}/configuration/systemd)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_SYSTEMD_FROM} ${_SYSTEMD_TO}
)

DeployShapeComponent(${PROJECT_NAME})
