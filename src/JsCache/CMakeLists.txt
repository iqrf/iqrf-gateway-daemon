project(JsCache)

find_package(Boost REQUIRED COMPONENTS filesystem )
find_package(shapeware REQUIRED)

set(COMPONENT iqrf::JsCache)
DeclareShapeComponent(${COMPONENT})
AddShapeProvidedInterface(${COMPONENT} iqrf::IJsCacheService)
AddShapeRequiredInterface(${COMPONENT} iqrf::IIqrfDpaService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::IJsRenderService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ILaunchService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} iqrf::ISchedulerService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::IRestApiService MANDATORY SINGLE)
AddShapeRequiredInterface(${COMPONENT} shape::ITraceService MANDATORY MULTIPLE)
ConfigureShapeComponent(${COMPONENT} COMPONENT_HXX)

file(GLOB_RECURSE _HDRFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h  ${COMPONENT_HXX})
file(GLOB_RECURSE _SRCFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

source_group("Header Files" FILES ${_HDRFILES})
source_group("Source Files" FILES ${_SRCFILES})

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/DpaParser)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${clibdpa_INCLUDE_DIRS})

if(SHAPE_STATIC_LIBS)
    add_library(${PROJECT_NAME} STATIC ${_HDRFILES} ${_SRCFILES})
else()
    add_library(${PROJECT_NAME} SHARED ${_HDRFILES} ${_SRCFILES})
endif()

target_link_libraries(${PROJECT_NAME}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)

DeployShapeComponent(${PROJECT_NAME})

if(WIN32)
	install(
		FILES
			#"$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_filesystem-vc141-mt-gd-x64-1_70.dll"
			"$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_system-vc140-mt-gd-x64-1_66.dll"
		CONFIGURATIONS Debug
		DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	)

	install(
		FILES
			#"$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_filesystem-vc141-mt-x64-1_70.dll"
			"$<TARGET_FILE_DIR:${PROJECT_NAME}>/boost_system-vc140-mt-x64-1_66.dll"
		CONFIGURATIONS Release
		DESTINATION "\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_INSTALL_PREFIX}/bin"
	)
endif()
