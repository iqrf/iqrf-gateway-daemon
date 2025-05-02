project(iqrf-gateway-daemon-tests)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

enable_testing()

find_package(GTest QUIET)
if(GTest_FOUND)
	include_directories(${GTEST_INCLUDE_DIRS})
else()
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libraries/googletest googletest EXCLUDE_FROM_ALL)
endif()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${PROJECT_NAME} ${TEST_FILES})
target_link_libraries(${PROJECT_NAME} gtest gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
