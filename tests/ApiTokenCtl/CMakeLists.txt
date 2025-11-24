project(ApiTokenCtlTest)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${PROJECT_NAME} ${TEST_FILES})
add_test(NAME ${PROJECT_NAME} COMMAND ApiTokenCtlTest)

target_link_libraries(${PROJECT_NAME} PRIVATE
  apitokenctl
  GTest::gtest
  GTest::gmock
  GTest::gtest_main
  GTest::gmock_main
)

