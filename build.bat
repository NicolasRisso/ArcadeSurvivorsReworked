@echo off
setlocal EnableDelayedExpansion

:: Project Configuration
set PROJECT_NAME=ArcadeSurvival
set OUT_DIR=bin
set SRC_DIR=src

:: Ensure output directory exists
if not exist %OUT_DIR% mkdir %OUT_DIR%

echo Building %PROJECT_NAME%...

:: Compilation Flags
set "INCLUDES=-Iraylib/include -I%SRC_DIR%"
set "LIBS=-Lraylib/lib -lraylib -lgdi32 -lwinmm"

:: Compile src/main.c (and others if found in src)
set "SOURCES="
for /R %SRC_DIR% %%f in (*.c) do (
    set "SOURCES=!SOURCES! "%%f""
)

gcc !SOURCES! -o %OUT_DIR%\%PROJECT_NAME%.exe %INCLUDES% %LIBS%

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Build completed: %OUT_DIR%\%PROJECT_NAME%.exe
    
    :: Copy required Raylib DLL if it exists
    if exist raylib\lib\raylib.dll (
        copy /Y raylib\lib\raylib.dll %OUT_DIR%\ >nul
        echo [INFO] Copied raylib.dll to %OUT_DIR%
    ) else if exist raylib\raylib.dll (
        copy /Y raylib\raylib.dll %OUT_DIR%\ >nul
        echo [INFO] Copied raylib.dll to %OUT_DIR%
    )
    
    :: Sync Assets
    if exist assets (
        echo [INFO] Syncing assets...
        xcopy /E /I /Y assets %OUT_DIR%\assets >nul
    )
) else (
    echo.
    echo [ERROR] Build failed! Check the errors above.
)

echo.
pause
