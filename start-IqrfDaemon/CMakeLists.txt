project (start-IqrfDaemon)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG_WIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationWin/*)
file(GLOB_RECURSE _CONFIG_LIN ${CMAKE_CURRENT_SOURCE_DIR}/configurationLin/*)
file(GLOB_RECURSE _CONFIG_EX ${CMAKE_CURRENT_SOURCE_DIR}/configurationExamples/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration Win" FILES ${_CONFIG_WIN})
source_group("Configuration Lin" FILES ${_CONFIG_LIN})
source_group("Configuration Examples" FILES ${_CONFIG_EX})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#include_directories(${CMAKE_SOURCE_DIR}/launcher)

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG_WIN} ${_CONFIG_LIN} ${_CONFIG_EX})

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

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${_CONFIG_DIR} "${_CONFIG_DEST}/configuration"
)
