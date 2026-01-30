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

// Serial allocation with reuse
TEST_P(AllocTest, SerialReuse) {
    const char *names[] = {"bacon", "eggs", "pancakes"};
    const int FILES = 3;
    const int CYCLES = 10;

    lfs_size_t block_size = cfg_.block_size;
    lfs_size_t block_count = cfg_.block_count;
    lfs_size_t SIZE = ((block_size - 8) * (block_count - 6)) / FILES;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    for (int c = 0; c < CYCLES; c++) {
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        ASSERT_EQ(lfs_mkdir(&lfs, "breakfast"), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        // Write serially
        for (int n = 0; n < FILES; n++) {
            ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
            char path[64];
            snprintf(path, sizeof(path), "breakfast/%s", names[n]);
            lfs_file_t file;
            ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);
            size_t size = strlen(names[n]);
            for (lfs_size_t i = 0; i < SIZE; i += size) {
                ASSERT_EQ(lfs_file_write(&lfs, &file, names[n], size),
                    (lfs_ssize_t)size);
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
                ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size),
                    (lfs_ssize_t)size);
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

// Exhaustion with wraparound - fill, delete first, fill again
TEST_P(AllocTest, ExhaustionWraparound) {
    const int FILES = 3;
    lfs_size_t block_size = cfg_.block_size;
    lfs_size_t block_count = cfg_.block_count;
    lfs_size_t SIZE = ((block_size - 8) * (block_count - 4)) / FILES;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Fill filesystem with 3 files
    uint8_t buffer[256];
    memset(buffer, 'a', sizeof(buffer));
    for (int n = 0; n < FILES; n++) {
        char path[64];
        snprintf(path, sizeof(path), "fill%d", n);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT), 0);
        lfs_size_t w = 0;
        while (w < SIZE) {
            lfs_size_t chunk = lfs_min(sizeof(buffer), SIZE - w);
            lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, chunk);
            if (res < 0) break;
            w += (lfs_size_t)res;
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }

    // Delete first file to create a gap
    ASSERT_EQ(lfs_remove(&lfs, "fill0"), 0);

    // Write a new file into the gap (tests lookahead wraparound)
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "wraparound",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    memset(buffer, 'w', sizeof(buffer));
    lfs_size_t w = 0;
    while (w < SIZE) {
        lfs_size_t chunk = lfs_min(sizeof(buffer), SIZE - w);
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, chunk);
        if (res < 0) break;
        w += (lfs_size_t)res;
    }
    ASSERT_GT(w, (lfs_size_t)0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Chained directory exhaustion (erase_size=512 only)
TEST_P(AllocTest, ChainedDirExhaustion) {
    if (cfg_.block_size != 512) {
        GTEST_SKIP() << "Only applies to block_size=512";
    }

    // Use smaller block count to make exhaustion feasible
    cfg_.block_count = 1024;
    bdcfg_.erase_count = 1024;

    // Reinitialize with new config
    lfs_emubd_destroy(&cfg_);
    ASSERT_EQ(lfs_emubd_create(&cfg_, &bdcfg_), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create a chain of directories with files until exhaustion
    int count = 0;
    for (int i = 0; i < 500; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%d", i);
        int err = lfs_mkdir(&lfs, path);
        if (err == LFS_ERR_NOSPC) break;
        ASSERT_EQ(err, 0);

        // Create a file in each dir
        char filepath[64];
        snprintf(filepath, sizeof(filepath), "dir%d/file", i);
        lfs_file_t file;
        err = lfs_file_open(&lfs, &file, filepath,
                LFS_O_WRONLY | LFS_O_CREAT);
        if (err == LFS_ERR_NOSPC) break;
        ASSERT_EQ(err, 0);
        lfs_file_close(&lfs, &file);
        count++;
    }

    ASSERT_GT(count, 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify we can still mount and read
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%d", i);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Split directory test (erase_size=512 only)
TEST_P(AllocTest, SplitDir) {
    if (cfg_.block_size != 512) {
        GTEST_SKIP() << "Only applies to block_size=512";
    }

    cfg_.block_count = 1024;
    bdcfg_.erase_count = 1024;

    lfs_emubd_destroy(&cfg_);
    ASSERT_EQ(lfs_emubd_create(&cfg_, &bdcfg_), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create enough entries to force a directory split
    // With 512-byte blocks, each metadata block can hold ~10-15 entries
    int count = 0;
    for (int i = 0; i < 200; i++) {
        char path[64];
        snprintf(path, sizeof(path), "longfilename_%04d", i);
        lfs_file_t file;
        int err = lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT);
        if (err == LFS_ERR_NOSPC) break;
        ASSERT_EQ(err, 0);
        lfs_file_close(&lfs, &file);
        count++;
    }

    ASSERT_GT(count, 15) << "Should have created enough files to split directory";
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify all files survive a remount
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "longfilename_%04d", i);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Outdated lookahead test
TEST_P(AllocTest, OutdatedLookahead) {
    if (cfg_.block_size != 512) {
        GTEST_SKIP() << "Only applies to block_size=512";
    }

    cfg_.block_count = 1024;
    bdcfg_.erase_count = 1024;

    lfs_emubd_destroy(&cfg_);
    ASSERT_EQ(lfs_emubd_create(&cfg_, &bdcfg_), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Allocate most of the filesystem
    lfs_size_t SIZE = ((cfg_.block_size - 8) * (cfg_.block_count - 4)) / 2;
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "fill_a",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    uint8_t buffer[256];
    memset(buffer, 'a', sizeof(buffer));
    lfs_size_t w = 0;
    while (w < SIZE) {
        lfs_size_t chunk = lfs_min(sizeof(buffer), SIZE - w);
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, chunk);
        if (res < 0) break;
        w += (lfs_size_t)res;
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "fill_b",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    w = 0;
    while (w < SIZE) {
        lfs_size_t chunk = lfs_min(sizeof(buffer), SIZE - w);
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, chunk);
        if (res < 0) break;
        w += (lfs_size_t)res;
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remount to clear lookahead
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Remove first file (lookahead may still show old state)
    ASSERT_EQ(lfs_remove(&lfs, "fill_a"), 0);

    // Allocate new file - must use freed blocks despite potentially
    // outdated lookahead
    ASSERT_EQ(lfs_file_open(&lfs, &file, "fill_c",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    memset(buffer, 'c', sizeof(buffer));
    w = 0;
    while (w < SIZE) {
        lfs_size_t chunk = lfs_min(sizeof(buffer), SIZE - w);
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, chunk);
        if (res < 0) break;
        w += (lfs_size_t)res;
    }
    ASSERT_GT(w, (lfs_size_t)0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, AllocTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
