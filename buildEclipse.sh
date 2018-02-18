#!/bin/bash
# Script for building IQRF daemon on Linux machine

set -e

project=IqrfGwDaemon

#expected build dir structure
buildexp=build/Eclipse_CDT4-Unix_Makefiles

LIB_DIRECTORY=..
currentdir=$PWD
builddir=./${buildexp}

mkdir -p ${builddir}

#get path to Shape libs
shape=../../shape/${buildexp}
pushd ${shape}
shape=$PWD
popd

#get path to clibcdc libs
clibcdc=${LIB_DIRECTORY}/clibcdc/${buildexp}
pushd ${clibcdc}
clibcdc=$PWD
popd

#get path to clibspi libs
clibspi=${LIB_DIRECTORY}/clibspi/${buildexp}
pushd ${clibspi}
clibspi=$PWD
popd

#get path to clibdpa libs
clibdpa=${LIB_DIRECTORY}/clibdpa/${buildexp}
pushd ${clibdpa}
clibdpa=$PWD
popd

#get path to cutils libs
cutils=${LIB_DIRECTORY}/cutils/${buildexp}
pushd ${cutils}
cutils=$PWD
popd

#launch cmake to generate build environment
pushd ${builddir}
pwd
cmake -G "Eclipse CDT4 - Unix Makefiles" -Dshape_DIR:PATH=${shape} -Dclibcdc_DIR:PATH=${clibcdc} -Dclibspi_DIR:PATH=${clibspi} -Dclibdpa_DIR:PATH=${clibdpa} -Dcutils_DIR:PATH=${cutils} ${currentdir} -DCMAKE_BUILD_TYPE=Debug
popd

#build from generated build environment
cmake --build ${builddir}
