# Foccuss Desktop

An application that helps users maintain focus by blocking access to selected applications on Windows.

## Features

- List and detect installed applications
- Select applications to block
- Block applications by overlaying a window when they are launched
- Run as a background service to continuously monitor application launches
- Persist blocked application settings in SQLite database

## Technology Stack

- Qt 6 for the frontend UI
- SQLite for data persistence
- CMake for build system
- C++ as the primary programming language

## Building the Application

### Prerequisites

- Qt 6.2+ (with QtWidgets, QtCore, QtSql modules)
- CMake 3.16+
- C++17 compatible compiler (MSVC recommended for Windows)
- SQLite3 development libraries

### Build Instructions

1. Clone the repository
2. Create a build directory:
   ```
   mkdir build
   cd build
   ```
3. Configure with CMake:
   ```
   cmake ..
   ```
4. Build the project:
   ```
   cmake --build .
   ```

## Installation

After building, the application can be installed by running the installer that's generated in the `build/install` directory.

## Usage

1. Launch the Foccuss application
2. Select applications you want to block from the list
3. The blocking service will start automatically in the background
4. When you attempt to launch a blocked application, an overlay will appear

## License

[MIT License](LICENSE)
