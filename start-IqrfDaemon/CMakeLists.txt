project (start-IqrfDaemon)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG_WIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationWin/*)
file(GLOB_RECURSE _CONFIG_LIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationLin/*)
file(GLOB_RECURSE _JAVA_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/JavaScript/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration Win" FILES ${_CONFIG_WIN})
source_group("Configuration Lin" FILES ${_CONFIG_LIN})
source_group("JavaScript" FILES ${_JAVA_SCRIPT})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG_WIN} ${_CONFIG_LIN} ${_JAVA_SCRIPT})

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

#copy configuration
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${_CONFIG_DIR} "${_CONFIG_DEST}/configuration"
)

#copy schemes
set(_JSON_SCHEMES_FROM ${CMAKE_SOURCE_DIR}/libraries/iqrf-daemon-api/JsonSchemes/async-api-json-schemes)
set(_JSON_SCHEMES_TO ${_CONFIG_DEST}/configuration/JsonSchemes/async-api-json-schemes)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JSON_SCHEMES_FROM} ${_JSON_SCHEMES_TO}
)

#copy JavaScript/DaemonWrapper.js
set(_JAVA_SCRIPT_FROM ${CMAKE_CURRENT_SOURCE_DIR}/JavaScript)
set(_JAVA_SCRIPT_TO ${_CONFIG_DEST}/configuration/JavaScript)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${_JAVA_SCRIPT_FROM} ${_JAVA_SCRIPT_TO}
)
