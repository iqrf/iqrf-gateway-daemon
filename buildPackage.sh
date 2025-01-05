#!/bin/bash

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


# Script for building IQRF Gateway Daemon's package

set -e

# Generate Debian changelog
if [ ! -v BUILD ] || [ -z "$BUILD" ] || [ "$BUILD" != "release" ]; then
    gbp dch -a -S --ignore-branch
else
    gbp dch -a -R --ignore-branch
fi

# Build Debian package
debuild -e CCACHE_COMPILERCHECK -e CCACHE_COMPRESS -e CCACHE_BASEDIR -e CCACHE_DIR -b -uc -us -tc
