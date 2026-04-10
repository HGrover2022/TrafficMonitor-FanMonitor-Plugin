@echo off
REM Try to find vcvarsall.bat in common locations
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
)

rc.exe /v FANmoniter.rc
cl.exe /LD /O2 /MD /W3 /EHsc /utf-8 /D_WINDLL /DNDEBUG /DUNICODE /D_UNICODE FANmoniter.cpp FANmoniter.res /link /OUT:FANmoniter.dll user32.lib gdi32.lib advapi32.lib shell32.lib
del *.obj *.res *.exp *.lib

