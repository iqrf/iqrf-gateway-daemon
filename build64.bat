set project=IqrfGwDaemon

rem //expected build dir structure
set buildexp=build\\VS14_2015_x64

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

set ver=v1.0.0
set tms="%date% %time%"

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_TOOLCHAIN_FILE=%vcpkg% -Dshape_DIR:PATH=%shape% -Dshapeware_DIR:PATH=%shaparts% -DDAEMON_VERSION:STRING=%ver% -DBUILD_TIMESTAMP:STRING=%tms% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
