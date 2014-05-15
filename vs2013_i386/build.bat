@echo off
set path=C:\Program Files (x86)\MSBuild\12.0\Bin
MSBuild.exe SoloMiner.sln /p:Configuration=Release
