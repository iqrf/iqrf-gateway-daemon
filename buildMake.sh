#!/bin/bash

# Copyright 2015-2023 IQRF Tech s.r.o.
# Copyright 2019-2023 MICRORISC s.r.o.
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

# Script for building IQRF daemon on Linux machine

set -e

project=IqrfGwDaemon

#expected build dir structure
buildexp=build/Unix_Makefiles

LIB_DIRECTORY=..
currentdir=$PWD
builddir=./${buildexp}

mkdir -p ${builddir}

#get path to shape libs
shape=../shape/${buildexp}
pushd ${shape}
shape=$PWD
popd

#get path to shapeware libs
shapeware=../shapeware/${buildexp}
pushd ${shapeware}
shapeware=$PWD
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Unix Makefiles" -DBUILD_TESTING:BOOL=true -Dshape_DIR:PATH=${shape} -Dshapeware_DIR:PATH=${shapeware} ${currentdir} -DCMAKE_BUILD_TYPE=Debug
popd

if [ "$@" = 0 ]; then
	#build from generated build environment
	cmake --build ${builddir} --config Debug --target install
	cmake --build ${builddir} --config Release --target install
else
	cmake --build ${builddir} --config Debug --target "$@"
fi
