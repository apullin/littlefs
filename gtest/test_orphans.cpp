/*
 * Orphan recovery tests
 *
 * Note: Some original orphan tests require internal LFS access (lfs_fs_preporphans,
 * lfs_dir_fetch, etc.) and power-loss simulation. This file ports the non-reentrant
 * stress tests that use public API only.
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class OrphansTest : public LfsParametricTest {};

// Simple PRNG for test determinism
static uint32_t test_prng(uint32_t *state) {
    *state = *state * 1103515245 + 12345;
    return *state;
}

// Non-reentrant stress test for orphan handling
// Creates and removes directories randomly to stress orphan handling
TEST_P(OrphansTest, NonreentrantDepth1Files6) {
    const int FILES = 6;
    const int DEPTH = 1;
    const int CYCLES = 2000;

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

        // If it does not exist, create it, else destroy
        struct lfs_info info;
        int res = lfs_stat(&lfs, full_path, &info);
        if (res == LFS_ERR_NOENT) {
            // Create each directory in turn
            for (int d = 0; d < DEPTH; d++) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                int err = lfs_mkdir(&lfs, path);
                ASSERT_TRUE(err == 0 || err == LFS_ERR_EXIST);
            }

            // Verify directories
            for (int d = 0; d < DEPTH; d++) {
                char path[256];
                strncpy(path, full_path, sizeof(path));
                path[2*d+2] = '\0';
                ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
                ASSERT_STREQ(info.name, &path[2*d+1]);
                ASSERT_EQ(info.type, LFS_TYPE_DIR);
            }
        } else {
            // Verify is valid dir
            ASSERT_STREQ(info.name, &full_path[2*(DEPTH-1)+1]);
            ASSERT_EQ(info.type, LFS_TYPE_DIR);

            // Try to delete path in reverse order
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

// Stress test with more files
TEST_P(OrphansTest, NonreentrantDepth1Files26) {
    const int FILES = 26;
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

// Stress test with nested directories
TEST_P(OrphansTest, NonreentrantDepth3Files3) {
    const int FILES = 3;
    const int DEPTH = 3;
    const int CYCLES = 2000;

    // Skip if not enough blocks - depth 3 needs more blocks for nested directories
    // Original test also skips for DEPTH==3 && CACHE_SIZE != 64
    if (2 * FILES >= (int)cfg_.block_count || cfg_.block_count < 64) {
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

INSTANTIATE_TEST_SUITE_P(
    Geometries, OrphansTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
