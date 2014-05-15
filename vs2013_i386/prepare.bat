@echo off

pushd ..\..\boost_1_55_0
set BOOST_LIBRARYDIR=%CD%\stage32\lib
set BOOST_ROOT=%CD%
popd

pushd ..\..\wxWidgets-3.0.0
set wxWidgets_ROOT_DIR=%CD%
popd

set path=C:\ProgramData\cmake-2.8.12.2-win32-x86\bin
cmake.exe -G "Visual Studio 12" ..
