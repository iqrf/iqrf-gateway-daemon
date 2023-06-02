#!/bin/bash

pushd build/Unix_Makefiles
ctest -v
popd
