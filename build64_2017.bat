set project=IqrfGwDaemon

rem //expected build dir structure
set buildexp=build\\VS15_2017_x64

set currentdir=%cd%
set builddir=.\\%buildexp%
set libsdir=..\\

mkdir %builddir%

rem //get path to to Shape libs
set shape=..\\..\\shape\\%buildexp%
pushd %shape%
set shape=%cd%
popd

rem //get path to to Shape libs
set shapeware=..\\..\\shapeware\\%buildexp%
pushd %shapeware%
set shaparts=%cd%
popd

set vcpkg="c:/devel/vcpkg/scripts/buildsystems/vcpkg.cmake"

set ver=v2.0.0dev

set vcpkg_ver="2021-01-13-unknownhash" 
rem set vcpkg_ver="" 

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 15 2017 Win64" -DBUILD_TESTING:BOOL=true -DCMAKE_TOOLCHAIN_FILE=%vcpkg% -DVCPKG_VER=%vcpkg_ver% -Dshape_DIR:PATH=%shape% -Dshapeware_DIR:PATH=%shaparts% -DDAEMON_VERSION:STRING=%ver% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir% --config Debug --target install
cmake --build %builddir% --config Release --target install
