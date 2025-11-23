project(iqrf-gateway-daemon-tests)

enable_testing()

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
  nlohmann_json::nlohmann_json
  OpenSSL::SSL
  OpenSSL::Crypto
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES
  ENVIRONMENT "TESTS_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/"
)
