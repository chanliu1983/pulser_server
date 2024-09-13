# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(MainProject VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Check if the operating system is macOS
if(APPLE)
    # Set additional compiler flags for macOS
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBEVENT REQUIRED libevent)

    include_directories(${LIBEVENT_INCLUDE_DIRS})
    link_directories(${LIBEVENT_LIBRARY_DIRS})

    target_link_libraries(main ${LIBEVENT_LIBRARIES})
endif()

# Add the executable
add_executable(main main.cpp server.cpp conduits.cpp executor.cpp events.cpp compress.cpp sender.cpp)

# Link the required libraries
target_link_libraries(main PRIVATE event z pthread lz4)
# Compile with _DEBUG macro define
# target_compile_definitions(main PRIVATE _DEBUG)
