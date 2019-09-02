project (iqrfgd2)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE _CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/*)
file(GLOB_RECURSE _CONFIG_WIN_CDC ${CMAKE_CURRENT_SOURCE_DIR}/configuration-WinCdc/*)
file(GLOB_RECURSE _JAVA_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/javaScript/*)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})
source_group("Configuration" FILES ${_CONFIG})
source_group("Configuration" FILES ${_CONFIG_WIN_CDC})
source_group("javaScript" FILES ${_JAVA_SCRIPT})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${_HDRFILES} ${_SRCFILES} ${_CONFIG} ${_CONFIG_WIN_CDC} ${_JAVA_SCRIPT})

target_link_libraries(${PROJECT_NAME} 
  launcher
)

if(UNIX)
  target_link_libraries(${PROJECT_NAME} 
    dl
    pthread
  )
endif()
 
DeployShapeComponent(${PROJECT_NAME})

if(UNIX)
	list(APPEND CfgList "LinCdc" "LinSpi" "LinUart" "LinDeploy" "LinDeployRelease")
else()
	list(APPEND CfgList "WinCdc")
endif()

foreach(cfg ${CfgList})
	DeployShapeConfiguration(${PROJECT_NAME}-${cfg}
		${CMAKE_CURRENT_SOURCE_DIR}/configuration					configuration
		${CMAKE_CURRENT_SOURCE_DIR}/configuration-${cfg}			configuration
		${CMAKE_SOURCE_DIR}/api	                                    configuration/apiSchemas
		${CMAKE_CURRENT_SOURCE_DIR}/scheduler						configuration/scheduler
    	#${CMAKE_SOURCE_DIR}/src/start-IqrfDaemon/DB/init			configuration/DB/init
    	${CMAKE_CURRENT_SOURCE_DIR}/DB/init							configuration/DB/init
		${CMAKE_CURRENT_SOURCE_DIR}/metaData						configuration/metaData
		${CMAKE_CURRENT_SOURCE_DIR}/iqrfRepoCache					configuration/iqrfRepoCache
		${CMAKE_CURRENT_SOURCE_DIR}/javaScript						configuration/javaScript
		${CMAKE_CURRENT_SOURCE_DIR}/cfgSchemas						configuration/cfgSchemas
		${CMAKE_CURRENT_SOURCE_DIR}/systemd							configuration/systemd
	)
	DeployShapeConfigurationStartUp("${PROJECT_INSTALL_PREFIX}/bin/${PROJECT_NAME}" "${PROJECT_NAME}-${cfg}")
endforeach()

ConfigureMsvcProject("${PROJECT_INSTALL_PREFIX}/bin/${PROJECT_NAME}" "${PROJECT_NAME}-WinCdc")
