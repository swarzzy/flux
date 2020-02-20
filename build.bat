@echo off
set ExecArg=run
if "%1" == "%ExecArg%" (
pushd build
flux.exe
popd
goto end
)

set ObjOutDir=build\obj\
set BinOutDir=build\

IF NOT EXIST %BinOutDir% mkdir %BinOutDir%
IF NOT EXIST %ObjOutDir% mkdir %ObjOutDir%

ctime -begin ctime.ctm
del %BinOutDir%*.pdb >NUL 2>&1
set PdbMangleVal=%date:~6,4%%date:~3,2%%date:~0,2%%time:~1,1%%time:~3,2%%time:~6,2%

set CommonDefines=/D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DPLATFORM_WINDOWS /DUNICODE /D_UNICODE
set CommonCompilerFlags=/Gm- /fp:fast /GR- /nologo /diagnostics:classic /WX /std:c++17 /Zi /W3 /FS
set DebugCompilerFlags=/Od /RTC1 /MTd /Fd%BinOutDir% /DPBR_DEBUG
set ReleaseCompilerFlags=/O2 /MT
set PlatformLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /NOIMPLIB user32.lib gdi32.lib opengl32.lib winmm.lib /OUT:%BinOutDir%\win32_flux.exe /PDB:%BinOutDir%\win32_flux.pdb
set GameLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /DLL /OUT:%BinOutDir%\flux.dll  /PDB:%BinOutDir%\flux_%PdbMangleVal%.pdb

set ConfigCompilerFlags=%DebugCompilerFlags%

echo Building platform...
start /b "__flux_compilation__" cmd /c cl /DPLATFORM_CODE /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/win32_flux_platform.cpp /link %PlatformLinkerFlags%

echo Building game...
start /b /wait "__flux_compilation__" cmd /c cl /Fo%ObjOutDir% %CommonDefines% %CommonCompilerFlags% %ConfigCompilerFlags% src/flux_load.cpp /link %GameLinkerFlags%

ctime -end ctime.ctm
:end
