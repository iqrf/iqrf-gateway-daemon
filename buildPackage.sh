#!/bin/bash
# Script for building IQRF Gateway Daemon's package

set -e

export DAEMON_VERSION=v2.0.0

rm -rf shape shape-build shapeware shapeware-build
mkdir shape-build shapeware-build

# Clone shape repository
git clone https://github.com/logimic/shape.git
git -C shape submodule init
git -C shape submodule update
git -C checkout v1.0.0

# Clone shapeware repository
git clone https://github.com/logimic/shapeware.git
git -C shapeware submodule init
git -C shapeware submodule update
git -C checkout v1.0.0

# Build shape
cmake -Bshape-build -Hshape -DCMAKE_SKIP_INSTALL_RPATH=TRUE -DCMAKE_SKIP_BUILD_RPATH=TRUE -DCMAKE_SKIP_RPATH=TRUE
cmake --build shape-build

# Build shapeware
cmake -Bshapeware-build -Hshapeware -Dshape_DIR:PATH=`pwd`/shape-build -DLWS_STATIC_PIC:BOOL=TRUE \
-DLWS_WITH_SSL:BOOL=FALSE -DCMAKE_SKIP_INSTALL_RPATH=TRUE -DCMAKE_SKIP_BUILD_RPATH=TRUE -DCMAKE_SKIP_RPATH=TRUE
cmake --build shapeware-build

# Generate Debian changelog
if [ ! -v BUILD ] || [ -z "$BUILD" ] || [ "$BUILD" != "release" ]; then
    gbp dch -a -S
else
    gbp dch -a -R
fi

# Build Debian package
dpkg-buildpackage -b -rfakeroot -us -uc -tc
