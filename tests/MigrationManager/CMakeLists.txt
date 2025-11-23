project(MigrationManagerTest)

add_executable(${PROJECT_NAME} MigrationManagerTest.cpp)
add_test(NAME ${PROJECT_NAME} COMMAND MigrationManagerTest)

target_link_libraries(${PROJECT_NAME} PRIVATE
  SQLiteCpp
  GTest::gtest
  GTest::gmock
  GTest::gtest_main
  GTest::gmock_main
  MigrationManager
)

set_tests_properties(${PROJECT_NAME} PROPERTIES
  ENVIRONMENT "MIGRATIONS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/migrations/"
)
