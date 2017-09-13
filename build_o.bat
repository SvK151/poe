@echo off
@mkdir ..\build
@pushd ..\build
cl /favor /GA ..\code\win32_poeitempull.cpp Wininet.lib Kernel32.lib zdll.lib
popd