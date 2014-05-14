@echo off
set include=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\include;C:\Program Files (x86)\Windows Kits\8.1\Include\shared;C:\Program Files (x86)\Windows Kits\8.1\Include\um
set lib=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\amd64;C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x64
set path=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64;C:\Program Files (x86)\Windows Kits\8.1\bin\x64
nmake.exe
