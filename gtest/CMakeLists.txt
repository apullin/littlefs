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

# Test executable
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
)

target_link_libraries(lfs_tests
    PRIVATE
    lfs
    lfs_bd
    gtest_main
)

target_include_directories(lfs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(lfs_tests
    PROPERTIES
    TIMEOUT 300
)
