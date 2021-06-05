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

build-all: build-bionic build-focal

build-bionic:
	$(MAKE) -C bionic/ build-all

build-focal:
	$(MAKE) -C focal/ build-all

push-all: push-bionic push-focal

push-bionic:
	$(MAKE) -C bionic/ push-all

push-focal:
	$(MAKE) -C focal/ push-all
