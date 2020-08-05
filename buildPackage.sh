#!/bin/bash
# Script for building IQRF Gateway Daemon's package

set -e

# Generate Debian changelog
if [ ! -v BUILD ] || [ -z "$BUILD" ] || [ "$BUILD" != "release" ]; then
    gbp dch -a -S
else
    gbp dch -a -R
fi

# Build Debian package
debuild -e CCACHE_COMPILERCHECK -e CCACHE_COMPRESS -e CCACHE_BASEDIR -e CCACHE_DIR -e DAEMON_VERSION -b -uc -us -tc
