@echo off
echo Fixing build issues and building Foccuss...

REM Create icons
echo Creating icon files...
call create_icons.bat

REM Clean build directory
echo Cleaning build directory...
if exist build (
    rd /s /q build
)
mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    cd ..
    exit /b %ERRORLEVEL%
)

REM Build the project
echo Building with CMake...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    cd ..
    exit /b %ERRORLEVEL%
)

REM Copy additional DLLs if needed
echo Copying required DLLs...
copy "C:\Qt\6.9.0\msvc2022_64\bin\Qt6Core.dll" Release\ >nul 2>&1
copy "C:\Qt\6.9.0\msvc2022_64\bin\Qt6Gui.dll" Release\ >nul 2>&1
copy "C:\Qt\6.9.0\msvc2022_64\bin\Qt6Widgets.dll" Release\ >nul 2>&1
copy "C:\Qt\6.9.0\msvc2022_64\bin\Qt6Sql.dll" Release\ >nul 2>&1
mkdir Release\platforms >nul 2>&1
mkdir Release\styles >nul 2>&1
mkdir Release\sqldrivers >nul 2>&1
copy "C:\Qt\6.9.0\msvc2022_64\plugins\platforms\qwindows.dll" Release\platforms\ >nul 2>&1
copy "C:\Qt\6.9.0\msvc2022_64\plugins\styles\qwindowsvistastyle.dll" Release\styles\ >nul 2>&1
copy "C:\Qt\6.9.0\msvc2022_64\plugins\sqldrivers\qsqlite.dll" Release\sqldrivers\ >nul 2>&1
copy "C:\SQLite\lib\sqlite3.dll" Release\ >nul 2>&1

echo Build successful!
echo Executable location: %cd%\Release\Foccuss.exe
cd .. 