project (iqrf-daemon-tests)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################
# Testing
################################
  
# This adds another subdirectory, which has 'project(gtest)'.
add_subdirectory(${CMAKE_SOURCE_DIR}/external/googletest .build)

# Include the gtest library. gtest_SOURCE_DIR is available due to
# 'project(gtest)' above.
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

##############
# Unit Tests
##############

add_executable(taskqueue_test task_queue.cpp)

# Standard linking to gtest stuff.
target_link_libraries(taskqueue_test gtest gtest_main pthread)

# You can also omit NAME and COMMAND. The second argument could be some other
# test executable.
add_test(NAME TestTaskQueue  COMMAND $<TARGET_FILE:taskqueue_test>)