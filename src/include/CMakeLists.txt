set(incpath ${PROJECT_SOURCE_DIR}/include/)

file(GLOB ALL_HEADERS ${incpath}/*.h ${incpath}/*.hpp)

add_custom_target(all_include SOURCES ${ALL_HEADERS})
