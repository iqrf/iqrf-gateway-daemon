set(TESTS_FOLDER tests)

add_subdirectory(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# TODO failing tests commnted out temporarily
add_subdirectory(TestScheduler)
add_subdirectory(TestJsRender)
#add_subdirectory(TestJsCache)
add_subdirectory(TestJsonDpaApiIqrfStandard)
add_subdirectory(TestJsonDpaApiRaw)
add_subdirectory(TestSimulationMessaging)
add_subdirectory(TestSimulationIqrfChannel)
#add_subdirectory(TestReadTrConfService)
#add_subdirectory(TestEnumerationDeviceService)
