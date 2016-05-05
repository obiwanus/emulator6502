@echo off

pushd w:\os

set CommonCompilerFlags= -DLL -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4706 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1 -FC -Z7 -Fm
set CommonLinkerFlags= -incremental:no -opt:ref winmm.lib user32.lib gdi32.lib

if not defined DevEnvDir (
    call misc\shell.bat
)

IF NOT EXIST build mkdir build
pushd build

@del /Q *.pdb > NUL 2> NUL
@del /Q *.gmi > NUL 2> NUL

cl %CommonCompilerFlags% ..\vm\win32_vm.cpp /link %CommonLinkerFlags%

popd
popd