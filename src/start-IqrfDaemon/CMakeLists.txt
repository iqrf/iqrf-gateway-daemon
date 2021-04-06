project (iqrfgd2)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _CONFIG_WIN_CDC ${CMAKE_CURRENT_SOURCE_DIR}/configuration-WinCdc/*)
file(GLOB_RECURSE _JAVA_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/javaScript/*)

find_package(Boost REQUIRED COMPONENTS program_options)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_CONFIG})
source_group("Configuration" FILES ${_CONFIG_WIN_CDC})
source_group("javaScript" FILES ${_JAVA_SCRIPT})

if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
else()
	message(FATAL_ERROR "Boost libraries not found.")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG} ${_CONFIG_WIN_CDC} ${_JAVA_SCRIPT})

target_link_libraries(${PROJECT_NAME} 
  launcher
  ${Boost_LIBRARIES}
)

if(UNIX)
  target_link_libraries(${PROJECT_NAME} 
    dl
    pthread
  )
endif()
 
DeployShapeComponent(${PROJECT_NAME})

if(UNIX)
	list(APPEND CfgList "LinCdc" "LinSpi" "LinUart" "LinDeploy")
else()
	list(APPEND CfgList "WinCdc" "WinUartWin")
endif()

add_custom_target(build-time-scheduler-dir ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/scheduler)

foreach(cfg ${CfgList})
	DeployShapeConfiguration(${PROJECT_NAME}-${cfg}
		${CMAKE_CURRENT_SOURCE_DIR}/configuration					configuration
		${CMAKE_CURRENT_SOURCE_DIR}/configuration-${cfg}			configuration
		${CMAKE_SOURCE_DIR}/api										configuration/apiSchemas
		${CMAKE_CURRENT_SOURCE_DIR}/scheduler						configuration/scheduler
		${CMAKE_CURRENT_SOURCE_DIR}/schedulerSchema					configuration/schedulerSchemas
   		${CMAKE_CURRENT_SOURCE_DIR}/DB/init							configuration/DB/init
		${CMAKE_CURRENT_SOURCE_DIR}/metaData/meta_data.json			configuration/metaData/meta_data.json
		${CMAKE_CURRENT_SOURCE_DIR}/metaData/schema_meta_data.json	configuration/metaDataSchemas/schema_meta_data.json
		${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache					configuration/iqrfRepoCache
		${CMAKE_CURRENT_SOURCE_DIR}/javaScript						configuration/javaScript
		${CMAKE_CURRENT_SOURCE_DIR}/cfgSchemas						configuration/cfgSchemas
		${CMAKE_CURRENT_SOURCE_DIR}/systemd							configuration/systemd
	)
	DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/${PROJECT_NAME}" "-c" "${PROJECT_NAME}-${cfg}")
endforeach()

if(WIN32)
  if(${VCPKG_VER} STREQUAL "2021-01-13-unknownhash")
	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_program_options-vc141-mt-gd-x64-1_75.dll"
		  CONFIGURATIONS Debug
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )

	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_program_options-vc141-mt-x64-1_75.dll"
		  CONFIGURATIONS Release
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )
  else()
	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_program_options-vc141-mt-gd-x64-1_73.dll"
		  CONFIGURATIONS Debug
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )

	  install(
		  FILES
			  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_program_options-vc141-mt-x64-1_73.dll"
		  CONFIGURATIONS Release
		  DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	  )
  endif()
endif()

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/${PROJECT_NAME}" "${PROJECT_NAME}-WinCdc")
