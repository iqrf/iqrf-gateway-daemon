cmake_minimum_required(VERSION 3.18)

project(iqrfgd2-cache-updater)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(LIBZIP REQUIRED)

set(PROJECT_SOURCES
    "update-cache.cpp"
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${LIBZIP_LIBRARY}
    cpr::cpr
    nlohmann_json::nlohmann_json
    ValiJSON::valijson
    pthread
)

add_custom_target(iqrfgd2-cache-update
    COMMAND ${PROJECT_NAME} -p ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Updating IQRF repository cache"
    SOURCES ${PROJECT_SOURCES}
)

list(APPEND CfgList "LinCdc" "LinSpi" "LinUart" "LinDeploy")

foreach(cfg ${CfgList})
	DeployShapeConfiguration(iqrfgd2-${cfg}
		${CMAKE_CURRENT_SOURCE_DIR}/cache       configuration/iqrfRepoCache/cache
	)
endforeach()
