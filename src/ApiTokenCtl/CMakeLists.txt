cmake_minimum_required(VERSION 3.18)

project(iqrfgd2-tokenctl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(OpenSSL REQUIRED)

# create library
set(LIB_NAME apitokenctl)
file(GLOB_RECURSE LIB_SOURCES "*.cpp" "*.h")
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
SET(DB_HEADERS
  ${CMAKE_SOURCE_DIR}/src/DbEntities/models/api_token.hpp
  ${CMAKE_SOURCE_DIR}/src/DbEntities/repositories/api_token_repo.hpp
)
add_library(${LIB_NAME} ${LIB_SOURCES} ${DB_HEADERS})
target_include_directories(${LIB_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/src/ApiTokenCtl
  ${CMAKE_SOURCE_DIR}/src/DbEntities/models
  ${CMAKE_SOURCE_DIR}/src/DbEntities/repositories
)
target_link_libraries(${LIB_NAME} PUBLIC
  ${Boost_LIBRARIES}
  nlohmann_json::nlohmann_json
  OpenSSL::SSL
  OpenSSL::Crypto
  SQLiteCpp
)

# create executable
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME})
unset(LIB_NAME)
