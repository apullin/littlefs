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

# Test sources
target_sources(app PRIVATE
    src/main.c
    src/test_superblocks.c
    src/test_files.c
    src/test_dirs.c
    src/test_paths.c
    src/test_seek.c
    src/test_move.c
    src/test_truncate.c
    src/test_alloc.c
    src/test_attrs.c
    src/test_entries.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../test
)

# Disable littlefs assertions for ztest (they cause kernel panics)
# Use zassert instead for test assertions
target_compile_definitions(app PRIVATE
    LFS_NO_ASSERT
)
