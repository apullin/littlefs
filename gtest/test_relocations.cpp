/*
 * Block relocation tests
 *
 * These tests stress the block relocator by creating and removing
 * directories/files in constrained space conditions.
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class RelocationsTest : public LfsParametricTest {};

// Simple PRNG for test determinism
static uint32_t test_prng(uint32_t *state) {
    *state = *state * 1103515245 + 12345;
    return *state;
}

// Test for dangling split directory - fill most of filesystem then create/remove
TEST_P(RelocationsTest, DanglingSplitDir) {
    const int ITERATIONS = 20;
    const int COUNT = 10;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Fill up filesystem so only ~16 blocks are left
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "padding",
            LFS_O_CREAT | LFS_O_WRONLY), 0);

    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    while (cfg_.block_count - lfs_fs_size(&lfs) > 16) {
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, sizeof(buffer));
        if (res < 0) break;
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Make a child dir to use in bounded space
    ASSERT_EQ(lfs_mkdir(&lfs, "child"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int j = 0; j < ITERATIONS; j++) {
        // Create files
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                    LFS_O_CREAT | LFS_O_WRONLY), 0);
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        }

        // Verify
        lfs_dir_t dir;
        struct lfs_info info;
        ASSERT_EQ(lfs_dir_open(&lfs, &dir, "child"), 0);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
            ASSERT_STREQ(info.name, path);
        }
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
        ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

        if (j == ITERATIONS - 1) {
            break;
        }

        // Remove files
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_remove(&lfs, path), 0);
        }
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Final verification after remount
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "child"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    for (int i = 0; i < COUNT; i++) {
        char path[256];
        snprintf(path, sizeof(path), "test%03d_loooooooooooooooooong_name", i);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_STREQ(info.name, path);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Cleanup
    for (int i = 0; i < COUNT; i++) {
        char path[256];
        snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
        ASSERT_EQ(lfs_remove(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test for outdated head handling
TEST_P(RelocationsTest, OutdatedHead) {
    const int ITERATIONS = 20;
    const int COUNT = 10;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Fill up filesystem so only ~16 blocks are left
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "padding",
            LFS_O_CREAT | LFS_O_WRONLY), 0);

    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    while (cfg_.block_count - lfs_fs_size(&lfs) > 16) {
        lfs_ssize_t res = lfs_file_write(&lfs, &file, buffer, sizeof(buffer));
        if (res < 0) break;
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "child"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int j = 0; j < ITERATIONS; j++) {
        // Create files
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                    LFS_O_CREAT | LFS_O_WRONLY), 0);
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        }

        // Read directory while also writing to files
        lfs_dir_t dir;
        struct lfs_info info;
        ASSERT_EQ(lfs_dir_open(&lfs, &dir, "child"), 0);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
            ASSERT_STREQ(info.name, path);
            ASSERT_EQ(info.size, (lfs_size_t)0);

            // Write to the file while iterating
            snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_WRONLY), 0);
            ASSERT_EQ(lfs_file_write(&lfs, &file, "hi", 2), 2);
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        }
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);

        // Rewind and verify updated sizes
        ASSERT_EQ(lfs_dir_rewind(&lfs, &dir), 0);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
            ASSERT_STREQ(info.name, path);
            ASSERT_EQ(info.size, (lfs_size_t)2);

            // Write again
            snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_WRONLY), 0);
            ASSERT_EQ(lfs_file_write(&lfs, &file, "hi", 2), 2);
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        }
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);

        // Final rewind and verify
        ASSERT_EQ(lfs_dir_rewind(&lfs, &dir), 0);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
            ASSERT_STREQ(info.name, path);
            ASSERT_EQ(info.size, (lfs_size_t)2);
        }
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
        ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

        // Remove files for next iteration
        for (int i = 0; i < COUNT; i++) {
            char path[256];
            snprintf(path, sizeof(path), "child/test%03d_loooooooooooooooooong_name", i);
            ASSERT_EQ(lfs_remove(&lfs, path), 0);
        }
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Non-reentrant stress test for relocations
TEST_P(RelocationsTest, NonreentrantDepth1Files6) {
    const int FILES = 6;
    const int DEPTH = 1;
    const int CYCLES = 2000;

    // Skip if not enough blocks
    if (2 * FILES >= (int)cfg_.block_count) {
        GTEST_SKIP() << "Not enough blocks for this test";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    uint32_t prng = 1;
    const char alpha[] = "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < CYCLES; i++) {
        // Create random path
        char full_path[256];
        for (int d = 0; d < DEPTH; d++) {
            snprintf(&full_path[2*d], sizeof(full_path) - 2*d, "/%c",
                    alpha[test_prng(&prng) % FILES]);
        }

        struct lfs_info info;
        int res = lfs_stat(&lfs, full_path, &info);
        if (res == LFS_ERR_NOENT) {
            for (int d = 0; d < DEPTH; d++) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                int err = lfs_mkdir(&lfs, path);
                ASSERT_TRUE(err == 0 || err == LFS_ERR_EXIST);
            }

            for (int d = 0; d < DEPTH; d++) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
                ASSERT_STREQ(info.name, &path[2*d+1]);
                ASSERT_EQ(info.type, LFS_TYPE_DIR);
            }
        } else {
            ASSERT_STREQ(info.name, &full_path[2*(DEPTH-1)+1]);
            ASSERT_EQ(info.type, LFS_TYPE_DIR);

            for (int d = DEPTH-1; d >= 0; d--) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                int err = lfs_remove(&lfs, path);
                ASSERT_TRUE(err == 0 || err == LFS_ERR_NOTEMPTY);
            }

            ASSERT_EQ(lfs_stat(&lfs, full_path, &info), LFS_ERR_NOENT);
        }
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Non-reentrant stress test with renames
TEST_P(RelocationsTest, NonreentrantRenamesDepth1Files6) {
    const int FILES = 6;
    const int DEPTH = 1;
    const int CYCLES = 2000;

    // Skip if not enough blocks
    if (2 * FILES >= (int)cfg_.block_count) {
        GTEST_SKIP() << "Not enough blocks for this test";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    uint32_t prng = 1;
    const char alpha[] = "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < CYCLES; i++) {
        // Create random path
        char full_path[256];
        for (int d = 0; d < DEPTH; d++) {
            snprintf(&full_path[2*d], sizeof(full_path) - 2*d, "/%c",
                    alpha[test_prng(&prng) % FILES]);
        }

        struct lfs_info info;
        int res = lfs_stat(&lfs, full_path, &info);
        ASSERT_TRUE(res == 0 || res == LFS_ERR_NOENT);

        if (res == LFS_ERR_NOENT) {
            // Create directories
            for (int d = 0; d < DEPTH; d++) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                int err = lfs_mkdir(&lfs, path);
                ASSERT_TRUE(err == 0 || err == LFS_ERR_EXIST);
            }

            for (int d = 0; d < DEPTH; d++) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
                ASSERT_STREQ(info.name, &path[2*d+1]);
                ASSERT_EQ(info.type, LFS_TYPE_DIR);
            }
        } else {
            ASSERT_STREQ(info.name, &full_path[2*(DEPTH-1)+1]);
            ASSERT_EQ(info.type, LFS_TYPE_DIR);

            // Create new random path for rename
            char new_path[256];
            for (int d = 0; d < DEPTH; d++) {
                snprintf(&new_path[2*d], sizeof(new_path) - 2*d, "/%c",
                        alpha[test_prng(&prng) % FILES]);
            }

            res = lfs_stat(&lfs, new_path, &info);
            ASSERT_TRUE(res == 0 || res == LFS_ERR_NOENT);

            if (res == LFS_ERR_NOENT) {
                // Rename
                for (int d = 0; d < DEPTH; d++) {
                    char path[256];
                    char path2[256];
                    strncpy(path, full_path, sizeof(path));
                    path[2*d+2] = '\0';
                    strncpy(path2, new_path, sizeof(path2));
                    path2[2*d+2] = '\0';
                    int err = lfs_rename(&lfs, path, path2);
                    ASSERT_TRUE(err == 0 || err == LFS_ERR_NOTEMPTY);
                    if (err == 0) {
                        strncpy(path, path2, sizeof(path));
                    }
                }

                for (int d = 0; d < DEPTH; d++) {
                    char path[256];
                    strncpy(path, new_path, sizeof(path));
                    path[2*d+2] = '\0';
                    ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
                    ASSERT_STREQ(info.name, &path[2*d+1]);
                    ASSERT_EQ(info.type, LFS_TYPE_DIR);
                }

                ASSERT_EQ(lfs_stat(&lfs, full_path, &info), LFS_ERR_NOENT);
            } else {
                // Delete
                for (int d = DEPTH-1; d >= 0; d--) {
                    char path[256];
                    strncpy(path, full_path, sizeof(path));
                    path[2*d+2] = '\0';
                    int err = lfs_remove(&lfs, path);
                    ASSERT_TRUE(err == 0 || err == LFS_ERR_NOTEMPTY);
                }

                ASSERT_EQ(lfs_stat(&lfs, full_path, &info), LFS_ERR_NOENT);
            }
        }
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, RelocationsTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
