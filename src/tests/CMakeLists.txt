# Copyright 2015-2025 IQRF Tech s.r.o.
# Copyright 2019-2025 MICRORISC s.r.o.
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

set(TESTS_FOLDER tests)

add_subdirectory(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# TODO failing tests commnted out temporarily
#add_subdirectory(TestScheduler)
add_subdirectory(TestJsRender)
#add_subdirectory(TestJsCache)
add_subdirectory(TestJsonDpaApiIqrfStandard)
add_subdirectory(TestJsonDpaApiRaw)
add_subdirectory(TestSimulationMessaging)
add_subdirectory(TestSimulationIqrfChannel)
#add_subdirectory(TestReadTrConfService)
#add_subdirectory(TestEnumerationDeviceService)
