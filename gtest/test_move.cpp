/*
 * File and directory move/rename tests
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class MoveTest : public LfsParametricTest {};

// Basic file move
TEST_P(MoveTest, File) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "a"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "b"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "c"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "d"), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "a/hello",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "hola\n", 5), 5);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "bonjour\n", 8), 8);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "ohayo\n", 6), 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rename file from a/hello to c/hello
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "a/hello", "c/hello"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify directories
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    struct lfs_info info;

    // a/ should be empty (only . and ..)
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "a"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // c/ should have hello
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "c"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "hello");
    ASSERT_EQ(info.type, LFS_TYPE_REG);
    ASSERT_EQ(info.size, 5u+8u+6u);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Verify file contents
    ASSERT_EQ(lfs_file_open(&lfs, &file, "a/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "b/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "c/hello", LFS_O_RDONLY), 0);
    uint8_t buffer[1024];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 5), 5);
    ASSERT_EQ(memcmp(buffer, "hola\n", 5), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 8), 8);
    ASSERT_EQ(memcmp(buffer, "bonjour\n", 8), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 6), 6);
    ASSERT_EQ(memcmp(buffer, "ohayo\n", 6), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "d/hello", LFS_O_RDONLY), LFS_ERR_NOENT);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Noop rename (same name) - this is legal
TEST_P(MoveTest, Noop) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "hi"), 0);
    ASSERT_EQ(lfs_rename(&lfs, "hi", "hi"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "hi/hi"), 0);
    ASSERT_EQ(lfs_rename(&lfs, "hi/hi", "hi/hi"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "hi/hi/hi"), 0);
    ASSERT_EQ(lfs_rename(&lfs, "hi/hi/hi", "hi/hi/hi"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "hi/hi/hi", &info), 0);
    ASSERT_STREQ(info.name, "hi");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Basic directory move
TEST_P(MoveTest, Dir) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "a"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "b"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "c"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "d"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "a/hi"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "a/hi/hola"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "a/hi/bonjour"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "a/hi/ohayo"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rename directory
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "a/hi", "c/hi"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    struct lfs_info info;

    // a/ should be empty
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "a"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // c/ should have hi
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "c"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "hi");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Check subdirectories moved correctly
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "a/hi"), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "b/hi"), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "c/hi"), 0);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "..");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "bonjour");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "hola");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_STREQ(info.name, "ohayo");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "d/hi"), LFS_ERR_NOENT);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Multiple renames in sequence
TEST_P(MoveTest, StateStealing) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "a"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "b"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "c"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "d"), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "a/hello",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "hola\n", 5), 5);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "bonjour\n", 8), 8);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "ohayo\n", 6), 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Chain of renames: a -> b -> c -> d
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "a/hello", "b/hello"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "b/hello", "c/hello"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "c/hello", "d/hello"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify file is now only in d
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "a/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "b/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "c/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "d/hello", LFS_O_RDONLY), 0);
    uint8_t buffer[1024];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 5), 5);
    ASSERT_EQ(memcmp(buffer, "hola\n", 5), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 8), 8);
    ASSERT_EQ(memcmp(buffer, "bonjour\n", 8), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 6), 6);
    ASSERT_EQ(memcmp(buffer, "ohayo\n", 6), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remove intermediate directories and verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_remove(&lfs, "b"), 0);
    ASSERT_EQ(lfs_remove(&lfs, "c"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "a", &info), 0);
    ASSERT_EQ(lfs_stat(&lfs, "b", &info), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_stat(&lfs, "c", &info), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_stat(&lfs, "d", &info), 0);

    // File still accessible
    ASSERT_EQ(lfs_file_open(&lfs, &file, "a/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "b/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "c/hello", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "d/hello", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 5), 5);
    ASSERT_EQ(memcmp(buffer, "hola\n", 5), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 8), 8);
    ASSERT_EQ(memcmp(buffer, "bonjour\n", 8), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 6), 6);
    ASSERT_EQ(memcmp(buffer, "ohayo\n", 6), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Rename to overwrite existing file
TEST_P(MoveTest, Overwrite) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create two files
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "original",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "original content", 16), 16);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "newfile",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "new content", 11), 11);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rename newfile to original (overwriting)
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "newfile", "original"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify only one file exists with new content
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "newfile", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "original", LFS_O_RDONLY), 0);
    uint8_t buffer[32];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 11), 11);
    ASSERT_EQ(memcmp(buffer, "new content", 11), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Rename in same directory
TEST_P(MoveTest, SameDir) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "oldname",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "test content", 12), 12);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rename in same directory
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_rename(&lfs, "oldname", "newname"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "oldname", LFS_O_RDONLY), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "newname", LFS_O_RDONLY), 0);
    uint8_t buffer[32];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 12), 12);
    ASSERT_EQ(memcmp(buffer, "test content", 12), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Rename non-existent file should fail
TEST_P(MoveTest, NonExistent) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_rename(&lfs, "nonexistent", "something"), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Cannot rename directory over file or vice versa
TEST_P(MoveTest, TypeMismatch) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create a file and a directory
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "myfile",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "mydir"), 0);

    // Try to rename file over directory - should fail
    ASSERT_EQ(lfs_rename(&lfs, "myfile", "mydir"), LFS_ERR_ISDIR);

    // Try to rename directory over file - should fail
    ASSERT_EQ(lfs_rename(&lfs, "mydir", "myfile"), LFS_ERR_NOTDIR);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Rename directory over empty directory (allowed)
TEST_P(MoveTest, DirOverEmpty) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "emptydir"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "nonempty"), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "nonempty/file",
            LFS_O_CREAT | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "content", 7), 7);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Rename non-empty directory over empty directory is allowed
    ASSERT_EQ(lfs_rename(&lfs, "nonempty", "emptydir"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify - nonempty is gone, emptydir now has the file
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "nonempty", &info), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_stat(&lfs, "emptydir", &info), 0);
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "emptydir/file", LFS_O_RDONLY), 0);
    uint8_t buffer[16];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 7), 7);
    ASSERT_EQ(memcmp(buffer, "content", 7), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, MoveTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
