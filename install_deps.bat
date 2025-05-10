@echo off
echo Installing dependencies for Foccuss...

REM Create SQLite directories
if not exist C:\SQLite mkdir C:\SQLite
if not exist C:\SQLite\include mkdir C:\SQLite\include
if not exist C:\SQLite\lib mkdir C:\SQLite\lib

REM Download SQLite amalgamation
echo Downloading SQLite...
powershell -Command "Invoke-WebRequest -Uri 'https://www.sqlite.org/2023/sqlite-amalgamation-3430200.zip' -OutFile 'sqlite.zip'"
if %ERRORLEVEL% neq 0 (
    echo Failed to download SQLite!
    exit /b %ERRORLEVEL%
)

REM Unzip SQLite
echo Extracting SQLite...
powershell -Command "Expand-Archive -Path 'sqlite.zip' -DestinationPath 'sqlite_temp' -Force"
if %ERRORLEVEL% neq 0 (
    echo Failed to extract SQLite!
    exit /b %ERRORLEVEL%
)

REM Copy SQLite files
echo Copying SQLite files...
copy sqlite_temp\sqlite-amalgamation-3430200\sqlite3.h C:\SQLite\include\
copy sqlite_temp\sqlite-amalgamation-3430200\sqlite3.c C:\SQLite\include\

REM Generate SQLite3 library
echo Building SQLite library...
cd sqlite_temp\sqlite-amalgamation-3430200
cl sqlite3.c /O2 /DSQLITE_ENABLE_COLUMN_METADATA /DSQLITE_ENABLE_FTS5 /DSQLITE_ENABLE_RTREE /LD /Fe:sqlite3.dll
if %ERRORLEVEL% neq 0 (
    echo Failed to build SQLite! Make sure Visual Studio Developer Command Prompt is in your PATH.
    cd ..\..
    exit /b %ERRORLEVEL%
)

copy sqlite3.dll C:\SQLite\lib\
copy sqlite3.lib C:\SQLite\lib\
cd ..\..

REM Clean up
echo Cleaning up...
rmdir /s /q sqlite_temp
del sqlite.zip

echo Dependencies installed successfully. 