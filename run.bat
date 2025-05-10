@echo off
echo Running Foccuss...

if not exist build\Release\Foccuss.exe (
    echo Executable not found, building first...
    call build.bat
    if %ERRORLEVEL% neq 0 (
        echo Build failed!
        exit /b %ERRORLEVEL%
    )
)

echo Starting Foccuss application...
start "" "build\Release\Foccuss.exe" 