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

rem //get path to clibcdc libs
set clibcdc=%libsdir%\\clibcdc\\build\\Visual_Studio_14_2015\\x64
pushd %clibcdc%
set clibcdc=%cd%
popd

rem //get path to clibspi libs
set clibspi=%libsdir%\\clibspi\\build\\Visual_Studio_14_2015\\x64
pushd %clibspi%
set clibspi=%cd%
popd

rem //get path to cutils libs
set cutils=%libsdir%\\cutils\\build\\Visual_Studio_14_2015\\x64
pushd %cutils%
set cutils=%cd%
popd

set vcpkg-export="c:/devel/vcpkg/vcpkg-export-20180317-095906/scripts/buildsystems/vcpkg.cmake"
set vcpkg="c:/devel/vcpkg/scripts/buildsystems/vcpkg.cmake"

set ver=v1.0.0
set tms="%date% %time%"

rem //launch cmake to generate build environment
pushd %builddir%
cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_TOOLCHAIN_FILE=%vcpkg% -Dshape_DIR:PATH=%shape% -Dshapeware_DIR:PATH=%shaparts% -Dcutils_DIR:PATH=%cutils% -Dclibcdc_DIR:PATH=%clibcdc% -Dclibspi_DIR:PATH=%clibspi% -DDAEMON_VERSION:STRING=%ver% -DBUILD_TIMESTAMP:STRING=%tms% %currentdir%
popd

rem //build from generated build environment
cmake --build %builddir%
