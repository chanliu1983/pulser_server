# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(MainProject VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(main main.cpp server.cpp conduits.cpp executor.cpp)

# Link the required libraries
target_link_libraries(main PRIVATE event z pthread)