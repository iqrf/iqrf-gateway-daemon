#!/bin/bash
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
shape=../../shape/${buildexp}
pushd ${shape}
shape=$PWD
popd

#get path to shapeware libs
shapeware=../../shapeware/${buildexp}
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

