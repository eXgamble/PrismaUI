@echo off
rem Local build of our lean CEF Prisma (kolebynov 2.0.0-beta.3 + trims; private, for the Mantle list).
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul || exit /b 1
set "VCPKG_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg"
cd /d "%~dp0"
cmake --preset release || exit /b 2
cmake --build --preset release --parallel 16 || exit /b 3
echo BUILD OK
