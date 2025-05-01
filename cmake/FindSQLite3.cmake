# FindSQLite3.cmake
#
# Find the SQLite3 libraries
#
# This module defines:
# SQLite3_FOUND - System has SQLite3
# SQLite3_INCLUDE_DIRS - The SQLite3 include directories
# SQLite3_LIBRARIES - The libraries needed to use SQLite3
# SQLite3_VERSION - The version of SQLite3 found

# Try to use pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SQLite3 QUIET sqlite3)
endif()

# Find the include directory
find_path(SQLite3_INCLUDE_DIR 
    NAMES sqlite3.h
    PATHS
    ${PC_SQLite3_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIR}
    C:/SQLite/include
    /usr/include
    /usr/local/include
)

# Find the library
find_library(SQLite3_LIBRARY 
    NAMES sqlite3 libsqlite3
    PATHS
    ${PC_SQLite3_LIBRARY_DIRS}
    ${SQLite3_LIBRARY}
    C:/SQLite/lib
    /usr/lib
    /usr/local/lib
)

# Get version if possible
if(PC_SQLite3_VERSION)
    set(SQLite3_VERSION ${PC_SQLite3_VERSION})
elseif(EXISTS "${SQLite3_INCLUDE_DIR}/sqlite3.h")
    file(STRINGS "${SQLite3_INCLUDE_DIR}/sqlite3.h" _ver_line
        REGEX "^#define SQLITE_VERSION[ \t]+\"[0-9]+\\.[0-9]+\\.[0-9]+\"$")
    if(_ver_line)
        string(REGEX REPLACE "^#define SQLITE_VERSION[ \t]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\"$"
            "\\1" SQLite3_VERSION "${_ver_line}")
    endif()
endif()

# Standard handling of find_package
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
    REQUIRED_VARS SQLite3_LIBRARY SQLite3_INCLUDE_DIR
    VERSION_VAR SQLite3_VERSION
)

# Set the output variables
if(SQLite3_FOUND)
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
endif()

# Hide internal variables in GUI
mark_as_advanced(
    SQLite3_INCLUDE_DIR
    SQLite3_LIBRARY
) 