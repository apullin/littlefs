cmake_minimum_required(VERSION 3.14)
project(littlefs C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core littlefs library
add_library(lfs STATIC
    lfs.c
    lfs_util.c
)
target_include_directories(lfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Block device implementations
add_library(lfs_bd STATIC
    bd/lfs_emubd.c
    bd/lfs_rambd.c
    bd/lfs_filebd.c
)
target_link_libraries(lfs_bd PUBLIC lfs)
target_include_directories(lfs_bd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Tests
option(LFS_BUILD_TESTS "Build GoogleTest tests" ON)
if(LFS_BUILD_TESTS)
    add_subdirectory(gtest)
endif()
