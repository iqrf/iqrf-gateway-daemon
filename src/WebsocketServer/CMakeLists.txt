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

project(WebsocketServer)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_library(${PROJECT_NAME} OBJECT
  WebsocketServer.cpp
  WebsocketSession.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${Boost_LIBRARIES}
  OpenSSL::SSL
  OpenSSL::Crypto
)
