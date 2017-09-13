@echo off
@mkdir ..\build
@pushd ..\build
cl -Zi ..\code\win32_poeitempull.cpp Wininet.lib Kernel32.lib zdll.lib
popd