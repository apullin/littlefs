/*
 * Block allocator tests
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class AllocTest : public LfsParametricTest {};

// Parallel allocation test
TEST_P(AllocTest, Parallel) {
    const char *names[] = {"bacon", "eggs", "pancakes"};
    const int FILES = 3;
    lfs_file_t files[FILES];

    // Calculate size to use most of the filesystem
    lfs_size_t block_size = cfg_.block_size;
    lfs_size_t block_count = cfg_.block_count;
    lfs_size_t SIZE = ((block_size - 8) * (block_count - 6)) / FILES;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "breakfast"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Write files in parallel
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int n = 0; n < FILES; n++) {
        char path[64];
        snprintf(path, sizeof(path), "breakfast/%s", names[n]);
        ASSERT_EQ(lfs_file_open(&lfs, &files[n], path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);
    }
    for (int n = 0; n < FILES; n++) {
        size_t size = strlen(names[n]);
        for (lfs_size_t i = 0; i < SIZE; i += size) {
            ASSERT_EQ(lfs_file_write(&lfs, &files[n], names[n], size), (lfs_ssize_t)size);
        }
    }
    for (int n = 0; n < FILES; n++) {
        ASSERT_EQ(lfs_file_close(&lfs, &files[n]), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int n = 0; n < FILES; n++) {
        char path[64];
        snprintf(path, sizeof(path), "breakfast/%s", names[n]);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
        size_t size = strlen(names[n]);
        for (lfs_size_t i = 0; i < SIZE; i += size) {
            uint8_t buffer[64];
            ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
            ASSERT_EQ(memcmp(buffer, names[n], size), 0);
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Serial allocation test
TEST_P(AllocTest, Serial) {
    const char *names[] = {"bacon", "eggs", "pancakes"};
    const int FILES = 3;

    lfs_size_t block_size = cfg_.block_size;
    lfs_size_t block_count = cfg_.block_count;
    lfs_size_t SIZE = ((block_size - 8) * (block_count - 6)) / FILES;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "breakfast"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Write files serially
    for (int n = 0; n < FILES; n++) {
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        char path[64];
        snprintf(path, sizeof(path), "breakfast/%s", names[n]);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);
        size_t size = strlen(names[n]);
        for (lfs_size_t i = 0; i < SIZE; i += size) {
            ASSERT_EQ(lfs_file_write(&lfs, &file, names[n], size), (lfs_ssize_t)size);
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int n = 0; n < FILES; n++) {
        char path[64];
        snprintf(path, sizeof(path), "breakfast/%s", names[n]);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
        size_t size = strlen(names[n]);
        for (lfs_size_t i = 0; i < SIZE; i += size) {
            uint8_t buffer[64];
            ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
            ASSERT_EQ(memcmp(buffer, names[n], size), 0);
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Parallel allocation with reuse
TEST_P(AllocTest, ParallelReuse) {
    const char *names[] = {"bacon", "eggs", "pancakes"};
    const int FILES = 3;
    const int CYCLES = 2;
    lfs_file_t files[FILES];

    lfs_size_t block_size = cfg_.block_size;
    lfs_size_t block_count = cfg_.block_count;
    lfs_size_t SIZE = ((block_size - 8) * (block_count - 6)) / FILES;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    for (int c = 0; c < CYCLES; c++) {
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        ASSERT_EQ(lfs_mkdir(&lfs, "breakfast"), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        // Write
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        for (int n = 0; n < FILES; n++) {
            char path[64];
            snprintf(path, sizeof(path), "breakfast/%s", names[n]);
            ASSERT_EQ(lfs_file_open(&lfs, &files[n], path,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);
        }
        for (int n = 0; n < FILES; n++) {
            size_t size = strlen(names[n]);
            for (lfs_size_t i = 0; i < SIZE; i += size) {
                ASSERT_EQ(lfs_file_write(&lfs, &files[n], names[n], size), (lfs_ssize_t)size);
            }
        }
        for (int n = 0; n < FILES; n++) {
            ASSERT_EQ(lfs_file_close(&lfs, &files[n]), 0);
        }
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        // Verify
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        for (int n = 0; n < FILES; n++) {
            char path[64];
            snprintf(path, sizeof(path), "breakfast/%s", names[n]);
            lfs_file_t file;
            ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
            size_t size = strlen(names[n]);
            for (lfs_size_t i = 0; i < SIZE; i += size) {
                uint8_t buffer[64];
                ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
                ASSERT_EQ(memcmp(buffer, names[n], size), 0);
            }
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        }
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        // Remove
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        for (int n = 0; n < FILES; n++) {
            char path[64];
            snprintf(path, sizeof(path), "breakfast/%s", names[n]);
            ASSERT_EQ(lfs_remove(&lfs, path), 0);
        }
        ASSERT_EQ(lfs_remove(&lfs, "breakfast"), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }
}

// Exhaustion test - try to fill the filesystem
TEST_P(AllocTest, Exhaustion) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "exhaustion",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    lfs_size_t block_size = cfg_.block_size;
    lfs_size_t block_count = cfg_.block_count;
    lfs_size_t total_size = (block_size - 8) * (block_count - 4);

    uint8_t buffer[256];
    memset(buffer, 'x', sizeof(buffer));

    lfs_size_t written = 0;
    while (written < total_size) {
        lfs_size_t chunk = lfs_min(sizeof(buffer), total_size - written);
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, chunk);
        if (res < 0) {
            // Expected to fail at some point
            ASSERT_EQ(res, LFS_ERR_NOSPC);
            break;
        }
        written += res;
    }

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Directory exhaustion test
TEST_P(AllocTest, DirExhaustion) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Try to create many directories until we run out of space
    int count = 0;
    for (int i = 0; i < 1000; i++) {
        char path[32];
        snprintf(path, sizeof(path), "dir%d", i);
        int err = lfs_mkdir(&lfs, path);
        if (err < 0) {
            ASSERT_EQ(err, LFS_ERR_NOSPC);
            break;
        }
        count++;
    }

    // Should have created at least some directories
    ASSERT_GT(count, 0);

    // Verify we can still read them
    for (int i = 0; i < count; i++) {
        char path[32];
        snprintf(path, sizeof(path), "dir%d", i);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
        ASSERT_EQ(info.type, LFS_TYPE_DIR);
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, AllocTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
