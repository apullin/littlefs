/*
 * Inline file entry tests - tests for corner cases with neighboring inline files.
 * Note: These tests are designed for 512 byte cache/inline sizes but will still
 * pass with other sizes (just won't be testing the specific corner cases).
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class EntriesTest : public LfsParametricTest {};

// Helper to write a file with given content
static void write_file(lfs_t *lfs, const char *path, lfs_size_t size) {
    lfs_file_t file;
    uint8_t buffer[1024];
    memset(buffer, 'c', sizeof(buffer));
    ASSERT_EQ(lfs_file_open(lfs, &file, path,
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);
    ASSERT_EQ(lfs_file_write(lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(lfs_file_size(lfs, &file), (lfs_soff_t)size);
    ASSERT_EQ(lfs_file_close(lfs, &file), 0);
}

// Helper to read and verify a file
static void read_verify_file(lfs_t *lfs, const char *path, lfs_size_t size) {
    lfs_file_t file;
    uint8_t rbuffer[1024];
    uint8_t expected[1024];
    memset(expected, 'c', sizeof(expected));
    ASSERT_EQ(lfs_file_open(lfs, &file, path, LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(lfs, &file), (lfs_soff_t)size);
    ASSERT_EQ(lfs_file_read(lfs, &file, rbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(rbuffer, expected, size), 0);
    ASSERT_EQ(lfs_file_close(lfs, &file), 0);
}

// Test growing an inline file
TEST_P(EntriesTest, Grow) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Write 4 small files
    write_file(&lfs, "hi0", 20);
    write_file(&lfs, "hi1", 20);
    write_file(&lfs, "hi2", 20);
    write_file(&lfs, "hi3", 20);

    // Read hi1 at 20 bytes, then grow to 200
    read_verify_file(&lfs, "hi1", 20);
    write_file(&lfs, "hi1", 200);

    // Verify all files
    read_verify_file(&lfs, "hi0", 20);
    read_verify_file(&lfs, "hi1", 200);
    read_verify_file(&lfs, "hi2", 20);
    read_verify_file(&lfs, "hi3", 20);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test shrinking an inline file
TEST_P(EntriesTest, Shrink) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Write files: 20, 200, 20, 20
    write_file(&lfs, "hi0", 20);
    write_file(&lfs, "hi1", 200);
    write_file(&lfs, "hi2", 20);
    write_file(&lfs, "hi3", 20);

    // Read hi1 at 200 bytes, then shrink to 20
    read_verify_file(&lfs, "hi1", 200);
    write_file(&lfs, "hi1", 20);

    // Verify all files
    read_verify_file(&lfs, "hi0", 20);
    read_verify_file(&lfs, "hi1", 20);
    read_verify_file(&lfs, "hi2", 20);
    read_verify_file(&lfs, "hi3", 20);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test spilling inline files (all files exceed inline threshold)
TEST_P(EntriesTest, Spill) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Write 4 files all at 200 bytes
    write_file(&lfs, "hi0", 200);
    write_file(&lfs, "hi1", 200);
    write_file(&lfs, "hi2", 200);
    write_file(&lfs, "hi3", 200);

    // Verify all files
    read_verify_file(&lfs, "hi0", 200);
    read_verify_file(&lfs, "hi1", 200);
    read_verify_file(&lfs, "hi2", 200);
    read_verify_file(&lfs, "hi3", 200);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test push spill - one small file among larger files
TEST_P(EntriesTest, PushSpill) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Write: 200, 20, 200, 200
    write_file(&lfs, "hi0", 200);
    write_file(&lfs, "hi1", 20);
    write_file(&lfs, "hi2", 200);
    write_file(&lfs, "hi3", 200);

    // Read hi1 at 20, then grow to 200
    read_verify_file(&lfs, "hi1", 20);
    write_file(&lfs, "hi1", 200);

    // Verify all files at 200
    read_verify_file(&lfs, "hi0", 200);
    read_verify_file(&lfs, "hi1", 200);
    read_verify_file(&lfs, "hi2", 200);
    read_verify_file(&lfs, "hi3", 200);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test push spill with two large files following small
TEST_P(EntriesTest, PushSpillTwo) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Write: 200, 20, 200, 200, 200
    write_file(&lfs, "hi0", 200);
    write_file(&lfs, "hi1", 20);
    write_file(&lfs, "hi2", 200);
    write_file(&lfs, "hi3", 200);
    write_file(&lfs, "hi4", 200);

    // Read hi1 at 20, then grow to 200
    read_verify_file(&lfs, "hi1", 20);
    write_file(&lfs, "hi1", 200);

    // Verify all files
    read_verify_file(&lfs, "hi0", 200);
    read_verify_file(&lfs, "hi1", 200);
    read_verify_file(&lfs, "hi2", 200);
    read_verify_file(&lfs, "hi3", 200);
    read_verify_file(&lfs, "hi4", 200);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test dropping files
TEST_P(EntriesTest, Drop) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Write 4 files at 200 bytes
    write_file(&lfs, "hi0", 200);
    write_file(&lfs, "hi1", 200);
    write_file(&lfs, "hi2", 200);
    write_file(&lfs, "hi3", 200);

    struct lfs_info info;

    // Remove hi1
    ASSERT_EQ(lfs_remove(&lfs, "hi1"), 0);
    ASSERT_EQ(lfs_stat(&lfs, "hi1", &info), LFS_ERR_NOENT);
    read_verify_file(&lfs, "hi0", 200);
    read_verify_file(&lfs, "hi2", 200);
    read_verify_file(&lfs, "hi3", 200);

    // Remove hi2
    ASSERT_EQ(lfs_remove(&lfs, "hi2"), 0);
    ASSERT_EQ(lfs_stat(&lfs, "hi2", &info), LFS_ERR_NOENT);
    read_verify_file(&lfs, "hi0", 200);
    read_verify_file(&lfs, "hi3", 200);

    // Remove hi3
    ASSERT_EQ(lfs_remove(&lfs, "hi3"), 0);
    ASSERT_EQ(lfs_stat(&lfs, "hi3", &info), LFS_ERR_NOENT);
    read_verify_file(&lfs, "hi0", 200);

    // Remove hi0
    ASSERT_EQ(lfs_remove(&lfs, "hi0"), 0);
    ASSERT_EQ(lfs_stat(&lfs, "hi0", &info), LFS_ERR_NOENT);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test creating a file with a too-big name
TEST_P(EntriesTest, CreateTooBig) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    char path[256];
    memset(path, 'm', 200);
    path[200] = '\0';

    lfs_file_t file;
    uint8_t wbuffer[1024];
    uint8_t rbuffer[1024];
    lfs_size_t size = 400;

    ASSERT_EQ(lfs_file_open(&lfs, &file, path,
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);
    memset(wbuffer, 'c', size);
    ASSERT_EQ(lfs_file_write(&lfs, &file, wbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(rbuffer, wbuffer, size), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test resizing a file to too-big
TEST_P(EntriesTest, ResizeTooBig) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    char path[256];
    memset(path, 'm', 200);
    path[200] = '\0';

    lfs_file_t file;
    uint8_t wbuffer[1024];
    uint8_t rbuffer[1024];

    // Write small file first
    lfs_size_t size = 40;
    ASSERT_EQ(lfs_file_open(&lfs, &file, path,
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);
    memset(wbuffer, 'c', size);
    ASSERT_EQ(lfs_file_write(&lfs, &file, wbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Read and verify
    ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(rbuffer, wbuffer, size), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Resize to large
    size = 400;
    ASSERT_EQ(lfs_file_open(&lfs, &file, path,
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);
    memset(wbuffer, 'c', size);
    ASSERT_EQ(lfs_file_write(&lfs, &file, wbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Read and verify larger size
    ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(rbuffer, wbuffer, size), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, EntriesTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
