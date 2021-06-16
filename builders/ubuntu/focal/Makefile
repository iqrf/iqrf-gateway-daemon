# Copyright 2015-2021 IQRF Tech s.r.o.
# Copyright 2019-2021 MICRORISC s.r.o.
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

DISTRIBUTION := ubuntu
REPOSITORY := iqrftech/iqrf-gateway-daemon-builder
VERSION := focal

all: build-all push-all

build-all: build-amd64 build-arm64 build-armhf

build-amd64:
	docker build -f amd64.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-amd64 .

build-arm64:
	docker build -f arm64.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-arm64 .


build-armhf:
	docker build -f armhf.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-armhf .

push-all: push-amd64 push-arm64 push-armhf

push-amd64:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-amd64

push-arm64:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-arm64

push-armhf:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-armhf
