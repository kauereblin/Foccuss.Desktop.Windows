cmake_minimum_required(VERSION 3.16)
project(Foccuss VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add our custom CMake modules path
#set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Qt
set(Qt6_DIR "C:/Qt/6.9.0/msvc2022_64/lib/cmake/Qt6")
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64")
find_package(Qt6 COMPONENTS Widgets Core Sql REQUIRED)

# SQLite
set(SQLite3_INCLUDE_DIR "C:/SQLite/include")
set(SQLite3_LIBRARY "C:/SQLite/lib/sqlite3.lib")
find_package(SQLite3 REQUIRED)

# Display paths for debugging
message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "SQLite3_INCLUDE_DIRS: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3_LIBRARIES: ${SQLite3_LIBRARIES}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLite3_INCLUDE_DIRS}
)

# Windows-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

# Define source files
set(SOURCES
    src/main.cpp
    
    # UI components
    src/ui/mainwindow.cpp
    src/ui/blockoverlay.cpp
    src/ui/applistmodel.cpp
    
    # Core functionality
    src/core/appdetector.cpp
    src/core/appmonitor.cpp
    
    # Service functionality
    src/service/winservice.cpp
    
    # Data management
    src/data/database.cpp
    src/data/appmodel.cpp
    src/data/blockTimeSettingsModel.cpp
)

# Define header files
set(HEADERS
    src/ui/mainwindow.h
    src/ui/blockoverlay.h
    src/ui/applistmodel.h
    src/core/appdetector.h
    src/core/appmonitor.h
    src/service/winservice.h
    src/data/database.h
    src/data/appmodel.h
    src/data/blockTimeSettingsModel.h
    include/Common.h
    include/ForwardDeclarations.h
    include/QtVersionCheck.h
)

# Resource files
set(RESOURCES
    resources/resources.qrc
)

# Create dummy icon files if they don't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png" "DUMMY")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/block_icon.png")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/block_icon.png" "DUMMY")
endif()

# Create executable
add_executable(Foccuss ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(Foccuss PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Sql
    ${SQLite3_LIBRARIES}
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(Foccuss PRIVATE
        psapi.lib          # Process Status API
        ole32.lib          # OLE API
        shell32.lib        # Shell API
        advapi32.lib       # Advanced Windows API
        user32.lib         # User Interface API 
    )
endif()

# Set VS working directory to be the binary output directory
set_target_properties(Foccuss PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Copy Qt DLLs to the build directory
if(WIN32)
    add_custom_command(TARGET Foccuss POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Sql>
            C:/Qt/6.9.0/msvc2022_64/plugins/sqldrivers/qsqlite.dll
            $<TARGET_FILE_DIR:Foccuss>
    )
endif()

# Install rules
install(TARGETS Foccuss
    RUNTIME DESTINATION bin
)

# Include subdirectories if needed
# add_subdirectory(tests) 