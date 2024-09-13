# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(MainProject VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(main main.cpp server.cpp conduits.cpp executor.cpp events.cpp compress.cpp sender.cpp multicast.cpp setting.cpp inih/ini.c inih/ini.h)

# Check if the operating system is macOS
if(APPLE)
    # Set additional compiler flags for macOS
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBEVENT REQUIRED libevent)

    include_directories(${LIBEVENT_INCLUDE_DIRS})
    include_directories(/opt/homebrew/Cellar/rapidjson/1.1.0/include())
    link_directories(${LIBEVENT_LIBRARY_DIRS})

    set(LZ4_INCLUDE_DIR "/opt/homebrew/Cellar/lz4/1.10.0/include")
    set(LZ4_LIBRARY "/opt/homebrew/Cellar/lz4/1.10.0/lib/liblz4.dylib")

    include_directories(${LZ4_INCLUDE_DIR})
    target_link_libraries(main PRIVATE ${LZ4_LIBRARY} 
    ${LIBEVENT_LIBRARY_DIRS}/libevent_pthreads-2.1.7.dylib 
    ${LIBEVENT_LIBRARY_DIRS}/libevent_openssl-2.1.7.dylib 
    ${LIBEVENT_LIBRARY_DIRS}/libevent_core-2.1.7.dylib
    ${LIBEVENT_LIBRARY_DIRS}/libevent_extra-2.1.7.dylib
    ${LIBEVENT_LIBRARY_DIRS}/libevent-2.1.7.dylib
    ${LIBEVENT_LIBRARY_DIRS}/libevent_openssl-2.1.7.dylib
    )

    # Set zlib paths
    set(ZLIB_INCLUDE_DIR "/opt/homebrew/opt/zlib/include")
    set(ZLIB_LIBRARY "/opt/homebrew/opt/zlib/lib/libz.dylib")

    include_directories(${ZLIB_INCLUDE_DIR})

    target_link_libraries(main PRIVATE ${ZLIB_LIBRARY})
else()
    # Link the required libraries
    target_link_libraries(main PRIVATE event z pthread lz4)
endif()

# Compile with _DEBUG macro define
# target_compile_definitions(main PRIVATE _DEBUG)
