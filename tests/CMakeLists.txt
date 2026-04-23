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

project(iqrf-gateway-daemon-tests)

enable_testing()

add_subdirectory(ApiTokenCtl)
add_subdirectory(MetadataParser)
add_subdirectory(MigrationManager)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

find_package(GTest CONFIG REQUIRED COMPONENTS GTest GMock)
find_package(OpenSSL REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")


add_executable(${PROJECT_NAME} ${TEST_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE
  GTest::gtest
  GTest::gmock
  GTest::gtest_main
  GTest::gmock_main
  date
  nlohmann_json::nlohmann_json
  OpenSSL::SSL
  OpenSSL::Crypto
  SQLiteCpp
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES
  ENVIRONMENT "TESTS_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/"
)
