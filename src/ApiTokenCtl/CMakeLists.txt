# Copyright 2015-2026 IQRF Tech s.r.o.
# Copyright 2019-2026 MICRORISC s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.18)

project(iqrfgd2-tokenctl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Boost CONFIG REQUIRED COMPONENTS program_options)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)

# create library
set(LIB_NAME apitokenctl)
file(GLOB_RECURSE LIB_SOURCES "*.cpp" "*.h")
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
SET(DB_HEADERS
  ${CMAKE_SOURCE_DIR}/src/DbEntities/models/api_token.hpp
  ${CMAKE_SOURCE_DIR}/src/DbEntities/repositories/api_token_repo.hpp
)
include_directories(${CMAKE_SOURCE_DIR}/libraries/date/include)
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
  SQLite::SQLite3
  SQLiteCpp
)

# create executable
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME})
unset(LIB_NAME)
