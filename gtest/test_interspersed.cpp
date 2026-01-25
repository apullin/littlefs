/*
 * Interspersed/concurrent file operations tests
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class InterspersedTest : public LfsParametricTest {};

// Interspersed file writes
TEST_P(InterspersedTest, Files) {
    const int SIZE = 100;
    const int FILES = 10;
    const char alphas[] = "abcdefghijklmnopqrstuvwxyz";

    lfs_t lfs;
    lfs_file_t files[FILES];

    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Open all files
    for (int j = 0; j < FILES; j++) {
        char path[8];
        snprintf(path, sizeof(path), "%c", alphas[j]);
        ASSERT_EQ(lfs_file_open(&lfs, &files[j], path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    }

    // Write interleaved
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < FILES; j++) {
            ASSERT_EQ(lfs_file_write(&lfs, &files[j], &alphas[j], 1), 1);
        }
    }

    // Close all files
    for (int j = 0; j < FILES; j++) {
        ASSERT_EQ(lfs_file_close(&lfs, &files[j]), 0);
    }

    // Verify directory contents
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    for (int j = 0; j < FILES; j++) {
        char path[8];
        snprintf(path, sizeof(path), "%c", alphas[j]);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_STREQ(info.name, path);
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        ASSERT_EQ(info.size, (lfs_size_t)SIZE);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Re-open and read interleaved
    for (int j = 0; j < FILES; j++) {
        char path[8];
        snprintf(path, sizeof(path), "%c", alphas[j]);
        ASSERT_EQ(lfs_file_open(&lfs, &files[j], path, LFS_O_RDONLY), 0);
    }

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < FILES; j++) {
            uint8_t buffer[1];
            ASSERT_EQ(lfs_file_read(&lfs, &files[j], buffer, 1), 1);
            ASSERT_EQ(buffer[0], (uint8_t)alphas[j]);
        }
    }

    for (int j = 0; j < FILES; j++) {
        ASSERT_EQ(lfs_file_close(&lfs, &files[j]), 0);
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Interspersed writes with removes
TEST_P(InterspersedTest, RemoveFiles) {
    const int SIZE = 100;
    const int FILES = 10;
    const char alphas[] = "abcdefghijklmnopqrstuvwxyz";

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create files
    for (int j = 0; j < FILES; j++) {
        char path[8];
        snprintf(path, sizeof(path), "%c", alphas[j]);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        for (int i = 0; i < SIZE; i++) {
            ASSERT_EQ(lfs_file_write(&lfs, &file, &alphas[j], 1), 1);
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remove files while writing another
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "zzz",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    for (int j = 0; j < FILES; j++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, "~", 1), 1);
        ASSERT_EQ(lfs_file_sync(&lfs, &file), 0);

        char path[8];
        snprintf(path, sizeof(path), "%c", alphas[j]);
        ASSERT_EQ(lfs_remove(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Verify only zzz remains
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "zzz");
    ASSERT_EQ(info.type, LFS_TYPE_REG);
    ASSERT_EQ(info.size, (lfs_size_t)FILES);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Verify contents
    ASSERT_EQ(lfs_file_open(&lfs, &file, "zzz", LFS_O_RDONLY), 0);
    for (int i = 0; i < FILES; i++) {
        uint8_t buffer[1];
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 1), 1);
        ASSERT_EQ(buffer[0], (uint8_t)'~');
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Remove file while others are open
TEST_P(InterspersedTest, RemoveInconveniently) {
    const int SIZE = 100;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t files[3];
    ASSERT_EQ(lfs_file_open(&lfs, &files[0], "e",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &files[1], "f",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &files[2], "g",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    // Write first half
    for (int i = 0; i < SIZE/2; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &files[0], "e", 1), 1);
        ASSERT_EQ(lfs_file_write(&lfs, &files[1], "f", 1), 1);
        ASSERT_EQ(lfs_file_write(&lfs, &files[2], "g", 1), 1);
    }

    // Remove middle file while still open
    ASSERT_EQ(lfs_remove(&lfs, "f"), 0);

    // Write second half
    for (int i = 0; i < SIZE/2; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &files[0], "e", 1), 1);
        // files[1] is orphaned but still valid
        ASSERT_EQ(lfs_file_write(&lfs, &files[1], "f", 1), 1);
        ASSERT_EQ(lfs_file_write(&lfs, &files[2], "g", 1), 1);
    }

    ASSERT_EQ(lfs_file_close(&lfs, &files[0]), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &files[1]), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &files[2]), 0);

    // Verify only e and g exist
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "e");
    ASSERT_EQ(info.size, (lfs_size_t)SIZE);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "g");
    ASSERT_EQ(info.size, (lfs_size_t)SIZE);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // f should not exist
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "f", LFS_O_RDONLY), LFS_ERR_NOENT);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, InterspersedTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
