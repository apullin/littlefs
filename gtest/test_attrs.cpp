/*
 * Extended attribute tests
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>

class AttrsTest : public LfsParametricTest {};

// Basic get/set attributes
TEST_P(AttrsTest, GetSet) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "hello"), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "hello/hello",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "hello", 5), 5);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Set attributes
    ASSERT_EQ(lfs_setattr(&lfs, "hello", 'A', "aaaa", 4), 0);
    ASSERT_EQ(lfs_setattr(&lfs, "hello", 'B', "bbbbbb", 6), 0);
    ASSERT_EQ(lfs_setattr(&lfs, "hello", 'C', "ccccc", 5), 0);

    // Get attributes
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'A', buffer, 4), 4);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'B', buffer + 4, 6), 6);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'C', buffer + 10, 5), 5);
    ASSERT_EQ(memcmp(buffer, "aaaa", 4), 0);
    ASSERT_EQ(memcmp(buffer + 4, "bbbbbb", 6), 0);
    ASSERT_EQ(memcmp(buffer + 10, "ccccc", 5), 0);

    // Set empty attribute
    ASSERT_EQ(lfs_setattr(&lfs, "hello", 'B', "", 0), 0);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'B', buffer + 4, 6), 0);

    // Remove attribute
    ASSERT_EQ(lfs_removeattr(&lfs, "hello", 'B'), 0);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'B', buffer + 4, 6), LFS_ERR_NOATTR);

    // Restore attribute
    ASSERT_EQ(lfs_setattr(&lfs, "hello", 'B', "dddddd", 6), 0);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'B', buffer + 4, 6), 6);
    ASSERT_EQ(memcmp(buffer + 4, "dddddd", 6), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify persistence
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    memset(buffer, 0, sizeof(buffer));
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'A', buffer, 4), 4);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'B', buffer + 4, 6), 6);
    ASSERT_EQ(lfs_getattr(&lfs, "hello", 'C', buffer + 10, 5), 5);
    ASSERT_EQ(memcmp(buffer, "aaaa", 4), 0);
    ASSERT_EQ(memcmp(buffer + 4, "dddddd", 6), 0);
    ASSERT_EQ(memcmp(buffer + 10, "ccccc", 5), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Attributes on root directory
TEST_P(AttrsTest, Root) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));

    // Set attributes on root
    ASSERT_EQ(lfs_setattr(&lfs, "/", 'A', "aaaa", 4), 0);
    ASSERT_EQ(lfs_setattr(&lfs, "/", 'B', "bbbbbb", 6), 0);

    // Get attributes
    ASSERT_EQ(lfs_getattr(&lfs, "/", 'A', buffer, 4), 4);
    ASSERT_EQ(lfs_getattr(&lfs, "/", 'B', buffer + 4, 6), 6);
    ASSERT_EQ(memcmp(buffer, "aaaa", 4), 0);
    ASSERT_EQ(memcmp(buffer + 4, "bbbbbb", 6), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify persistence
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    memset(buffer, 0, sizeof(buffer));
    ASSERT_EQ(lfs_getattr(&lfs, "/", 'A', buffer, 4), 4);
    ASSERT_EQ(lfs_getattr(&lfs, "/", 'B', buffer + 4, 6), 6);
    ASSERT_EQ(memcmp(buffer, "aaaa", 4), 0);
    ASSERT_EQ(memcmp(buffer + 4, "bbbbbb", 6), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// File-level attributes via opencfg
TEST_P(AttrsTest, FileConfig) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "hello"), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "hello/hello",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "hello", 5), 5);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));

    struct lfs_attr attrs[] = {
        {'A', buffer, 4},
        {'B', buffer + 4, 6},
    };
    struct lfs_file_config filecfg;
    memset(&filecfg, 0, sizeof(filecfg));
    filecfg.attrs = attrs;
    filecfg.attr_count = 2;

    // Write attributes via file config
    ASSERT_EQ(lfs_file_opencfg(&lfs, &file, "hello/hello",
            LFS_O_WRONLY, &filecfg), 0);
    memcpy(buffer, "aaaa", 4);
    memcpy(buffer + 4, "bbbbbb", 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Read back
    memset(buffer, 0, 10);
    ASSERT_EQ(lfs_file_opencfg(&lfs, &file, "hello/hello",
            LFS_O_RDONLY, &filecfg), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(memcmp(buffer, "aaaa", 4), 0);
    ASSERT_EQ(memcmp(buffer + 4, "bbbbbb", 6), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, AttrsTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
