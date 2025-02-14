cmake_minimum_required(VERSION 3.16)
project(MakeMKVBackupApp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Find required Qt packages
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Automatically include all source files from lib and app
file(GLOB LIB_SOURCES "source/lib/*.cpp")
file(GLOB APP_SOURCES "source/app/*.cpp")

# Build the static library
add_library(makemkvbackup STATIC ${LIB_SOURCES})
target_include_directories(makemkvbackup PUBLIC source/lib)

# Build the application
add_executable(makemkv-backup ${APP_SOURCES})
target_link_libraries(makemkv-backup makemkvbackup Qt5::Widgets)

# Installation paths
install(TARGETS makemkvbackup makemkv-backup
    LIBRARY DESTINATION /usr/local/lib
    ARCHIVE DESTINATION /usr/local/lib
    RUNTIME DESTINATION /usr/local/bin
)
install(DIRECTORY source/lib/ DESTINATION /usr/local/include
    FILES_MATCHING PATTERN "*.h"
)
