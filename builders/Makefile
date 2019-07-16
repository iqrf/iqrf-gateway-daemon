# Copyright 2019 IQRF Tech s.r.o.
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

all: build-all push-all

build-all: build-debian build-ubuntu

build-debian:
	$(MAKE) -C debian/ build-all

build-ubuntu:
	$(MAKE) -C ubuntu/ build-all

push-all: push-debian push-ubuntu

push-debian:
	$(MAKE) -C debian/ push-all

push-ubuntu:
	$(MAKE) -C ubuntu/ push-all
