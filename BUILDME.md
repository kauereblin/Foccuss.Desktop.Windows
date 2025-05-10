# Building Foccuss Desktop Application

This document explains how to build and run the Foccuss Desktop application.

## Prerequisites

Before building, make sure you have:

1. **Visual Studio** (2019 or 2022) with C++ development tools
2. **Qt 6.2.0+** installed (ideally 6.9.0 for msvc2022_64)
3. **CMake 3.16+** installed and in your PATH
4. **SQLite3** development files (see the installation script below)

## Setting Up Dependencies

### Installing SQLite3

Run the provided installation script:

```
install_deps.bat
```

This will:
1. Download SQLite 3.43.2
2. Extract the files
3. Compile the SQLite library
4. Install it to C:\SQLite for the build system to find

Note: The script requires Visual Studio Developer Command Prompt to be accessible.

### Using Existing SQLite3

If you already have SQLite3 installed, make sure:
- Header files are in C:\SQLite\include
- Library files are in C:\SQLite\lib
- The library file is named sqlite3.lib

## Building the Application

### Easy Method

Run the provided build script:

```
build.bat
```

This will:
1. Create a build directory
2. Configure CMake
3. Build the application in Release mode

### Manual Build

If you prefer to build manually:

```
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Running the Application

### Using the Run Script

To run the application, use:

```
run.bat
```

This will:
1. Build the application if needed
2. Launch the executable

### Manual Launch

You can also run the application directly:

```
.\build\Release\Foccuss.exe
```

## Troubleshooting

### Qt Path Issues

If CMake cannot find Qt, edit CMakeLists.txt to set the correct path:

```cmake
set(Qt6_DIR "C:/path/to/your/Qt/version/msvc_version/lib/cmake/Qt6")
set(CMAKE_PREFIX_PATH "C:/path/to/your/Qt/version/msvc_version")
```

### Missing DLLs

If the application crashes due to missing DLLs, make sure:
1. Qt's bin directory is in your PATH, or
2. Copy all required DLLs to the same directory as the executable

The build script should automatically copy the basic Qt DLLs.

### Icon Issues

The application requires PNG icon files in the resources/icons directory:
- app_icon.png
- block_icon.png

If these are missing, replace the placeholder files with real PNG files. 