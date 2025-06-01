project(DbEntities)

#file(GLOB_RECURSE HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

#add_library(${PROJECT_NAME} INTERFACE ${HDR_FILES})

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/repositories
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    SQLiteCpp
)
