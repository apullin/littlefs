include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Main test executable (uses lfs library)
add_executable(lfs_tests
    lfs_test_fixture.cpp
    test_bd.cpp
    test_superblocks.cpp
    test_files.cpp
    test_dirs.cpp
    test_seek.cpp
    test_attrs.cpp
    test_truncate.cpp
    test_move.cpp
    test_paths.cpp
    test_interspersed.cpp
    test_alloc.cpp
    test_entries.cpp
    test_orphans.cpp
    test_relocations.cpp
    test_badblocks.cpp
    test_exhaustion.cpp
    test_shrink.cpp
    test_wear_leveling.cpp
)

target_link_libraries(lfs_tests
    PRIVATE
    lfs
    lfs_bd
    gtest_main
)

target_include_directories(lfs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Internal tests executable (uses C wrapper for internal function access)
# These tests need access to internal static functions, which are exposed
# through a C wrapper (lfs_test_internal.c) that includes lfs.c directly.
add_executable(lfs_internal_tests
    lfs_test_internal.c
    test_evil.cpp
    test_powerloss.cpp
    test_compat.cpp
)

# lfs_test_internal.c #includes lfs.c directly and cannot include its own
# header (type conflicts), so suppress the missing-prototypes warning.
set_source_files_properties(lfs_test_internal.c PROPERTIES
    COMPILE_FLAGS "-Wno-missing-prototypes"
)

target_link_libraries(lfs_internal_tests
    PRIVATE
    lfs_bd
    gtest_main
)

# Include paths for internal tests: need access to lfs.c and its dependencies
target_include_directories(lfs_internal_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}  # For lfs.c, lfs.h, lfs_util.h
)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(lfs_tests
    PROPERTIES
    TIMEOUT 300
)
gtest_discover_tests(lfs_internal_tests
    PROPERTIES
    TIMEOUT 300
)
