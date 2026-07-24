@echo off

cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo could not find visual studio
    exit /b 1
)

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1

if not exist "build\obj" mkdir "build\obj"

if not exist "build\bin" mkdir "build\bin"

echo generating proxy exports...

powershell -NoProfile -ExecutionPolicy Bypass -File "tools\gen_proxy.ps1"

if errorlevel 1 (
    echo proxy export generation failed
    exit /b 1
)

echo compiling resources...

rc.exe /nologo /fo"build\obj\resources.res" src\resources\resources.rc

if errorlevel 1 (
    echo resource compile failed
    exit /b 1
)

echo compiling...

cl.exe /nologo /O2 /Gy /GR- /std:c++17 /EHsc /MT /W3 /LD ^
    /I"src" ^
    src\dllmain.cpp ^
    src\engine\engine.cpp ^
    src\patches\crc\crc.cpp ^
    src\patches\demonware\demonware.cpp ^
    src\patches\oob\oob.cpp ^
    src\patches\mspreload\mspreload.cpp ^
    src\patches\callvote\callvote.cpp ^
    src\patches\presence\presence.cpp ^
    src\patches\lobbymsg\lobbymsg.cpp ^
    src\patches\infoleak\infoleak.cpp ^
    src\patches\inventory\inventory.cpp ^
    src\patches\markup\markup.cpp ^
    src\patches\paragon\paragon.cpp ^
    src\patches\p2p\p2p.cpp ^
    src\patches\netchan\netchan.cpp ^
    src\patches\steamqol\steamqol.cpp ^
    src\patches\antiquit\antiquit.cpp ^
    src\patches\hotkeys\hotkeys.cpp ^
    src\features\logo\logo.cpp ^
    src\utils\hook\hook.cpp ^
    src\utils\hook\lde.cpp ^
    src\utils\log\log.cpp ^
    src\utils\exceptions\exceptions.cpp ^
    src\utils\mem\mem.cpp ^
    src\utils\resource\resource.cpp ^
    /Fo"build\obj\\" ^
    /Fe"build\bin\t7-release.dll" ^
    /link /DLL /OPT:REF /OPT:ICF /OUT:"build\bin\t7-release.dll" ^
    build\obj\resources.res ^
    ole32.lib ^
    dbghelp.lib ^
    user32.lib

if errorlevel 1 (
    echo build failed
    exit /b 1
)

echo build succeeded: build\bin\t7-release.dll

echo deploying to bo3...

powershell -NoProfile -ExecutionPolicy Bypass -File "tools\deploy.ps1"
