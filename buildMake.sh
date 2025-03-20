#!/bin/bash

# IQRF Gateway Daemon
# Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

#build from generated build environment
cmake --build ${builddir} --config Debug --target install
cmake --build ${builddir} --config Release --target install
