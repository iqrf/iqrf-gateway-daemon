project(Test_include)

set(incpath ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB ALL_TEST_HEADERS ${incpath}/*.h ${incpath}/*.hpp)

add_custom_target(${PROJECT_NAME} SOURCES ${ALL_TEST_HEADERS})

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER ${TESTS_FOLDER})
