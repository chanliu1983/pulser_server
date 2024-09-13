# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(MainProject VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(main main.cpp server.cpp conduits.cpp executor.cpp 
events.cpp compress.cpp sender.cpp multicast.cpp setting.cpp inih/ini.c inih/ini.h 
db.cpp)

# Find LevelDB
find_package(leveldb REQUIRED)

include_directories(${LEVELDB_INCLUDE_DIRS})
link_directories(${LEVELDB_LIBRARY_DIRS})

# Check if the operating system is macOS
if(APPLE)
    # Set additional compiler flags for macOS
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBEVENT REQUIRED libevent)

    include_directories(${LIBEVENT_INCLUDE_DIRS})
    include_directories(/opt/homebrew/Cellar/rapidjson/1.1.0/include())
    link_directories(${LIBEVENT_LIBRARY_DIRS})

    set(LZ4_INCLUDE_DIR "/opt/homebrew/opt/lz4/include")
    set(LZ4_LIBRARY "/opt/homebrew/opt/lz4/lib/liblz4.dylib")

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

    set(leveldb_LIBRARY "/opt/homebrew/opt/leveldb/lib/libleveldb.dylib")
    set(LEVELDB_INCLUDE_DIRS "/opt/homebrew/opt/leveldb/include")

    include_directories(${LEVELDB_INCLUDE_DIRS})

    target_link_libraries(main PRIVATE ${ZLIB_LIBRARY} ${leveldb_LIBRARY})
else()
    # Link the required libraries
    target_link_libraries(main PRIVATE event z pthread lz4 leveldb)
endif()

# Compile with _DEBUG macro define
# target_compile_definitions(main PRIVATE _DEBUG)
