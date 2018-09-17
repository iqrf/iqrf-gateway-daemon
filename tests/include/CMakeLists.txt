set(incpath ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB ALL_TEST_HEADERS ${incpath}/*.h ${incpath}/*.hpp)
add_custom_target(Test_include SOURCES ${ALL_TEST_HEADERS})
