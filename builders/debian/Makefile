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

build-all: build-buster build-bullseye

build-bullseye:
	$(MAKE) -C bullseye/ build-all

build-buster:
	$(MAKE) -C buster/ build-all

push-all: push-buster push-bullseye

push-bullseye:
	$(MAKE) -C bullseye/ push-all

push-buster:
	$(MAKE) -C buster/ push-all
