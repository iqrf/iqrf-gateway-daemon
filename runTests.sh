#!/bin/bash

pushd build/Unix_Makefiles
ctest -V
popd
