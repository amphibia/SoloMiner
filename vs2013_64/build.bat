@echo off
set path=C:\Program Files (x86)\MSBuild\12.0\Bin\amd64
MSBuild.exe SoloMiner.sln /p:Configuration=Release
