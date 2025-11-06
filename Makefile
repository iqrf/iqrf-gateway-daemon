BUILD_DIR=build/Unix_Makefiles
LIB_DIRECTORY=..
CURRENT_DIR=${PWD}
SHAPE_DIR=$(realpath -s ../shape/${BUILD_DIR})
SHAPEWARE_DIR=$(realpath -s ../shapeware/${BUILD_DIR})
PATH_PARAMS=-Dshape_DIR:PATH=${SHAPE_DIR} -Dshapeware_DIR:PATH=${SHAPEWARE_DIR} ${CURRENT_DIR} -DCMAKE_BUILD_TYPE=Debug
TEST_PARAMS=-DBUILD_TESTING:BOOL=true
COV_PARAMS=-DCODE_COVERAGE=1

.phony: all prepare_build build test

prepare_build:
	mkdir -p ${BUILD_DIR}

build: prepare_build
	cd ${BUILD_DIR} && cmake -G "Unix Makefiles" ${PATH_PARAMS}
	cmake --build ${BUILD_DIR} --config Debug --target install

test: prepare_build
	cd ${BUILD_DIR} && cmake -G "Unix Makefiles" ${PATH_PARAMS} ${TEST_PARAMS}
	cmake --build ${BUILD_DIR} --config Debug --target install
	cd ${BUILD_DIR}/tests && ctest -V

coverage: prepare_build
	cd ${BUILD_DIR} && cmake -G "Unix Makefiles" ${PATH_PARAMS} ${TEST_PARAMS} ${COV_PARAMS}
	cmake --build ${BUILD_DIR} --config Debug --target install
	cd ${BUILD_DIR}/tests && ctest -V --output-junit ../../ctest.xml
