/*
 * Directory operation tests - creation, removal, rename, listing
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class DirsTest : public LfsParametricTest {};

// Test root directory
TEST_P(DirsTest, Root) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_STREQ(info.name, ".");

    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_STREQ(info.name, "..");

    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Many directory creation (expanded N values)
TEST_P(DirsTest, ManyCreation) {
    const int N = 33;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify directories exist
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%03d", i);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_DIR);
        ASSERT_STREQ(info.name, path);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Directory removal
TEST_P(DirsTest, ManyRemoval) {
    const int N = 10;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "removeme%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remove all directories
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "removeme%03d", i);
        ASSERT_EQ(lfs_remove(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify empty
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Directory rename
TEST_P(DirsTest, ManyRename) {
    const int N = 10;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "test%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rename all directories
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < N; i++) {
        char oldpath[64], newpath[64];
        snprintf(oldpath, sizeof(oldpath), "test%03d", i);
        snprintf(newpath, sizeof(newpath), "tedd%03d", i);
        ASSERT_EQ(lfs_rename(&lfs, oldpath, newpath), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify renamed directories
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "tedd%03d", i);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_DIR);
        ASSERT_STREQ(info.name, path);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// File creation in directories
TEST_P(DirsTest, FileCreation) {
    const int N = 10;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "file%03d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify files
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "file%03d", i);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        ASSERT_STREQ(info.name, path);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Nested directories
TEST_P(DirsTest, Nested) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "potato"), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "burito",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "potato/baked"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "potato/sweet"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "potato/fried"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify nested structure
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "potato"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "baked");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "fried");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "sweet");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Try removing non-empty dir
    ASSERT_EQ(lfs_remove(&lfs, "potato"), LFS_ERR_NOTEMPTY);

    // Rename nested
    ASSERT_EQ(lfs_rename(&lfs, "potato", "hotpotato"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remove after emptying
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_remove(&lfs, "hotpotato/baked"), 0);
    ASSERT_EQ(lfs_remove(&lfs, "hotpotato/fried"), 0);
    ASSERT_EQ(lfs_remove(&lfs, "hotpotato/sweet"), 0);
    ASSERT_EQ(lfs_remove(&lfs, "hotpotato"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify final state
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "burito");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Error conditions
TEST_P(DirsTest, Errors) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "potato"), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "burito",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Duplicate creation
    ASSERT_EQ(lfs_mkdir(&lfs, "potato"), LFS_ERR_EXIST);
    ASSERT_EQ(lfs_mkdir(&lfs, "burito"), LFS_ERR_EXIST);

    // Open non-existent
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "tomato"), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "burito"), LFS_ERR_NOTDIR);

    // File on directory
    ASSERT_EQ(lfs_file_open(&lfs, &file, "potato", LFS_O_RDONLY), LFS_ERR_ISDIR);

    // Root operations
    ASSERT_EQ(lfs_mkdir(&lfs, "/"), LFS_ERR_EXIST);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "/", LFS_O_RDONLY), LFS_ERR_ISDIR);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Directory seek
TEST_P(DirsTest, Seek) {
    const int COUNT = 10;
    if ((unsigned)COUNT >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "hello"), 0);
    for (int i = 0; i < COUNT; i++) {
        char path[64];
        snprintf(path, sizeof(path), "hello/kitty%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Test seeking to each entry
    for (int j = 0; j < COUNT; j++) {
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        lfs_dir_t dir;
        ASSERT_EQ(lfs_dir_open(&lfs, &dir, "hello"), 0);

        struct lfs_info info;
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..

        for (int i = 0; i < j; i++) {
            ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        }

        lfs_soff_t pos = lfs_dir_tell(&lfs, &dir);
        ASSERT_GE(pos, 0);

        ASSERT_EQ(lfs_dir_seek(&lfs, &dir, pos), 0);
        char path[64];
        snprintf(path, sizeof(path), "kitty%03d", j);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_STREQ(info.name, path);

        // Rewind and verify
        ASSERT_EQ(lfs_dir_rewind(&lfs, &dir), 0);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_STREQ(info.name, ".");

        ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }
}

// Recursive removal
TEST_P(DirsTest, RecursiveRemove) {
    const int N = 5;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "prickly-pear"), 0);
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "prickly-pear/cactus%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, path), 0);
    }

    // Can't remove non-empty
    ASSERT_EQ(lfs_remove(&lfs, "prickly-pear"), LFS_ERR_NOTEMPTY);

    // Remove children then parent
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "prickly-pear/cactus%03d", i);
        ASSERT_EQ(lfs_remove(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_remove(&lfs, "prickly-pear"), 0);

    // Verify removed
    ASSERT_EQ(lfs_remove(&lfs, "prickly-pear"), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// File removal test
TEST_P(DirsTest, FileRemoval) {
    const int N = 14;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "file%03d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remove all files
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "file%03d", i);
        ASSERT_EQ(lfs_remove(&lfs, path), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify empty
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// File rename test
TEST_P(DirsTest, FileRename) {
    const int N = 14;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "test%03d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rename all files
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < N; i++) {
        char oldpath[64], newpath[64];
        snprintf(oldpath, sizeof(oldpath), "test%03d", i);
        snprintf(newpath, sizeof(newpath), "tedd%03d", i);
        ASSERT_EQ(lfs_rename(&lfs, oldpath, newpath), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify renamed files
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "tedd%03d", i);
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        ASSERT_STREQ(info.name, path);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Remove-read test: open dir, remove entry, continue reading
TEST_P(DirsTest, RemoveRead) {
    const int N = 10;
    if ((unsigned)N >= cfg_.block_count / 2) {
        GTEST_SKIP() << "Not enough blocks";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, path), 0);
    }

    // Open root and skip . and ..
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..

    // Read first entry, then remove it while dir is open
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "dir000");
    ASSERT_EQ(lfs_remove(&lfs, "dir000"), 0);

    // Should still be able to read remaining entries
    int count = 0;
    while (lfs_dir_read(&lfs, &dir, &info) == 1) {
        count++;
    }
    // We should get N-1 remaining entries
    ASSERT_EQ(count, N - 1);

    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, DirsTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
