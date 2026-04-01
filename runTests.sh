#!/bin/bash

pushd build/Unix_Makefiles
ctest --progress -V
popd
