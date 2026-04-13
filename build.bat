@echo off
set PROJECT_NAME=ArcadeSurvival
set OUT_DIR=bin
set COMPILER=gcc

if not exist %OUT_DIR% mkdir %OUT_DIR%

:: Collect all .c files in the current folder (main.c) and the src directory recursively
setlocal EnableDelayedExpansion
set "SOURCE_FILES="
for /R src %%f in (*.c) do (
    set "SOURCE_FILES=!SOURCE_FILES! "%%f""
)

echo Building %PROJECT_NAME%...

:: Check for local Raylib binaries
set "RAYLIB_INCLUDES=-Iraylib/include"
set "RAYLIB_LIBS=-Lraylib/lib -lraylib"

if not exist raylib\lib\libraylib.a (
    if not exist raylib\lib\raylib.lib (
        echo [INFO] Local Raylib binaries not found at raylib/lib.
        echo [INFO] Attempting to use system-installed Raylib...
        set "RAYLIB_INCLUDES="
        set "RAYLIB_LIBS=-lraylib"
    )
)

%COMPILER% !SOURCE_FILES! -o %OUT_DIR%/%PROJECT_NAME%.exe ^
    -Isrc %RAYLIB_INCLUDES% ^
    %RAYLIB_LIBS% ^
    -lgdi32 ^
    -lwinmm

if %ERRORLEVEL% equ 0 (
    echo Build successful! Binary is in %OUT_DIR%
    if exist raylib\lib\raylib.dll (
        copy raylib\lib\raylib.dll %OUT_DIR%\ >nul
        echo Copied raylib.dll to %OUT_DIR%
    ) else if exist raylib\raylib.dll (
        copy raylib\raylib.dll %OUT_DIR%\ >nul
        echo Copied raylib.dll to %OUT_DIR%
    )
    
    :: Copy assets directory
    if exist assets (
        xcopy /E /I /Y assets %OUT_DIR%\assets >nul
        echo Copied assets to %OUT_DIR%\assets
    )
) else (
    echo Build failed!
)
pause
