# Zephyr CMakeLists.txt for littlefs ztest
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(littlefs_test)

# littlefs core library
target_sources(app PRIVATE
    ../lfs.c
    ../lfs_util.c
)

# Zephyr flash block device wrapper
target_sources(app PRIVATE
    src/lfs_test_bd_zephyr.c
)

# Test main
target_sources(app PRIVATE
    src/main.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../test
)
