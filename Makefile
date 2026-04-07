BUILD_DIR=build/Unix_Makefiles
LIB_DIRECTORY=..
CURRENT_DIR=${PWD}
SHAPE_DIR=$(realpath -s ../shape/${BUILD_DIR})
PATH_PARAMS=-Dshape_DIR:PATH=${SHAPE_DIR} ${CURRENT_DIR} -DCMAKE_BUILD_TYPE=Debug
FLAGS=-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
TEST_PARAMS=-DBUILD_TESTING:BOOL=true
COV_PARAMS=-DCODE_COVERAGE=1

.phony: all prepare_build build test

prepare_build:
	mkdir -p ${BUILD_DIR}

build: prepare_build
	cd ${BUILD_DIR} && cmake -G "Unix Makefiles" ${PATH_PARAMS} ${FLAGS}
	cmake --build ${BUILD_DIR} --config Debug --target install

test: prepare_build
	cd ${BUILD_DIR} && cmake -G "Unix Makefiles" ${PATH_PARAMS} ${FLAGS} ${TEST_PARAMS}
	cmake --build ${BUILD_DIR} --config Debug --target install
	cd ${BUILD_DIR}/tests && ctest -V

coverage: prepare_build
	cd ${BUILD_DIR} && cmake -G "Unix Makefiles" ${PATH_PARAMS} ${FLAGS} ${TEST_PARAMS} ${COV_PARAMS}
	cmake --build ${BUILD_DIR} --config Debug --target install
	cd ${BUILD_DIR} && ctest -V --output-junit ../../ctest.xml
	cd ${BUILD_DIR} && gcovr --html-details --exclude-unreachable-branches --print-summary -o ../../coverage/ --root ../.. --object-directory .
