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

DISTRIBUTION := debian
REPOSITORY := iqrftech/iqrf-gateway-daemon-builder
VERSION := bullseye

all: build-all push-all

build-all: build-amd64 build-arm64 build-armel build-armhf build-i386

build-amd64:
	docker build --no-cache -f amd64.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-amd64 .

build-arm64:
	docker build --no-cache -f arm64.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-arm64 .

build-armel:
	docker build --no-cache -f armel.Dockerfile --no-cache -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-armel .

build-armhf:
	docker build --no-cache -f armhf.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-armhf .

build-i386:
	docker build --no-cache -f i386.Dockerfile -t $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-i386 .

push-all: push-amd64 push-arm64 push-armel push-armhf push-i386

push-amd64:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-amd64

push-arm64:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-arm64

push-armel:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-armel

push-armhf:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-armhf

push-i386:
	docker push $(REPOSITORY):$(DISTRIBUTION)-$(VERSION)-i386
