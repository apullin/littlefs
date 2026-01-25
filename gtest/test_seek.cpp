/*
 * File seek tests - positioning, boundary conditions
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class SeekTest : public LfsParametricTest {};

// Simple seek read
TEST_P(SeekTest, Read) {
    const int COUNT = 50;
    const int SKIP = 10;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    const char* data = "kittycatcat";
    size_t size = strlen(data);
    uint8_t buffer[1024];
    memcpy(buffer, data, size);

    for (int j = 0; j < COUNT; j++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Read and seek
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty", LFS_O_RDONLY), 0);

    lfs_soff_t pos = -1;
    for (int i = 0; i < SKIP; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(buffer, data, size), 0);
        pos = lfs_file_tell(&lfs, &file);
    }
    ASSERT_GE(pos, 0);

    // Seek back to pos and read
    ASSERT_EQ(lfs_file_seek(&lfs, &file, pos, LFS_SEEK_SET), pos);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(buffer, data, size), 0);

    // Rewind
    ASSERT_EQ(lfs_file_rewind(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(buffer, data, size), 0);

    // SEEK_CUR - current position is at size after reading one record
    ASSERT_EQ(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_CUR), (lfs_soff_t)size);
    ASSERT_EQ(lfs_file_seek(&lfs, &file, size, LFS_SEEK_CUR), (lfs_soff_t)(2 * size));

    // SEEK_END
    lfs_soff_t endpos = lfs_file_seek(&lfs, &file, -(lfs_soff_t)size, LFS_SEEK_END);
    ASSERT_GE(endpos, 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(buffer, data, size), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Simple seek write
TEST_P(SeekTest, Write) {
    const int COUNT = 50;
    const int SKIP = 10;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    const char* data = "kittycatcat";
    size_t size = strlen(data);
    uint8_t buffer[1024];
    memcpy(buffer, data, size);

    for (int j = 0; j < COUNT; j++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Open for read/write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty", LFS_O_RDWR), 0);

    lfs_soff_t pos = -1;
    for (int i = 0; i < SKIP; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(buffer, data, size), 0);
        pos = lfs_file_tell(&lfs, &file);
    }
    ASSERT_GE(pos, 0);

    // Seek and write different data
    const char* newdata = "doggodogdog";
    ASSERT_EQ(strlen(newdata), size);
    memcpy(buffer, newdata, size);
    ASSERT_EQ(lfs_file_seek(&lfs, &file, pos, LFS_SEEK_SET), pos);
    ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);

    // Read back the modified data
    ASSERT_EQ(lfs_file_seek(&lfs, &file, pos, LFS_SEEK_SET), pos);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(buffer, newdata, size), 0);

    // Verify original data at start
    ASSERT_EQ(lfs_file_rewind(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(buffer, data, size), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Out of bounds seek
TEST_P(SeekTest, OutOfBounds) {
    const int COUNT = 20;
    const int SKIP = 5;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    const char* data = "kittycatcat";
    size_t size = strlen(data);
    uint8_t buffer[1024];
    memcpy(buffer, data, size);

    for (int j = 0; j < COUNT; j++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty", LFS_O_RDWR), 0);

    // Verify size
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)(COUNT * size));

    // Seek past end
    lfs_soff_t pastend = (COUNT + SKIP) * size;
    ASSERT_EQ(lfs_file_seek(&lfs, &file, pastend, LFS_SEEK_SET), pastend);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);

    // Write past end creates hole
    const char* newdata = "porcupineee";
    memcpy(buffer, newdata, size);
    ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);

    // Read back
    ASSERT_EQ(lfs_file_seek(&lfs, &file, pastend, LFS_SEEK_SET), pastend);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(memcmp(buffer, newdata, size), 0);

    // Hole should be zeros
    ASSERT_EQ(lfs_file_seek(&lfs, &file, COUNT * size, LFS_SEEK_SET),
            (lfs_soff_t)(COUNT * size));
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    for (size_t i = 0; i < size; i++) {
        ASSERT_EQ(buffer[i], 0);
    }

    // Negative seek from current should fail if past start
    lfs_soff_t bad_offset = -((COUNT + SKIP) * (lfs_soff_t)size);
    ASSERT_EQ(lfs_file_seek(&lfs, &file, bad_offset, LFS_SEEK_CUR), LFS_ERR_INVAL);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Inline file seek
TEST_P(SeekTest, InlineWrite) {
    const unsigned SIZE = 32;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "tinykitty", LFS_O_RDWR | LFS_O_CREAT), 0);

    uint8_t buffer[26];
    memcpy(buffer, "abcdefghijklmnopqrstuvwxyz", 26);

    unsigned j = 0;
    for (unsigned i = 0; i < SIZE; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, &buffer[j++ % 26], 1), 1);
        ASSERT_EQ(lfs_file_tell(&lfs, &file), (lfs_soff_t)(i + 1));
        ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)(i + 1));
    }

    ASSERT_EQ(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)SIZE);

    unsigned k = 0;
    for (unsigned i = 0; i < SIZE; i++) {
        uint8_t c;
        ASSERT_EQ(lfs_file_read(&lfs, &file, &c, 1), 1);
        ASSERT_EQ(c, buffer[k++ % 26]);
    }

    ASSERT_EQ(lfs_file_sync(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Seek underflow/overflow
TEST_P(SeekTest, Underflow) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    uint8_t buffer[1024];
    const char* data = "kittycatcat";
    size_t size = strlen(data);
    memcpy(buffer, data, size);
    ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);

    // Underflow with SEEK_CUR
    ASSERT_EQ(lfs_file_seek(&lfs, &file, -(lfs_soff_t)(size + 10), LFS_SEEK_CUR), LFS_ERR_INVAL);
    ASSERT_EQ(lfs_file_seek(&lfs, &file, -LFS_FILE_MAX, LFS_SEEK_CUR), LFS_ERR_INVAL);

    // Underflow with SEEK_END
    ASSERT_EQ(lfs_file_seek(&lfs, &file, -(lfs_soff_t)(size + 10), LFS_SEEK_END), LFS_ERR_INVAL);

    // File pointer should not have changed
    ASSERT_EQ(lfs_file_tell(&lfs, &file), (lfs_soff_t)size);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

TEST_P(SeekTest, Overflow) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "kitty",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    uint8_t buffer[1024];
    const char* data = "kittycatcat";
    size_t size = strlen(data);
    memcpy(buffer, data, size);
    ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);

    // Seek to max
    ASSERT_EQ(lfs_file_seek(&lfs, &file, LFS_FILE_MAX, LFS_SEEK_SET), LFS_FILE_MAX);

    // Overflow with SEEK_CUR
    ASSERT_EQ(lfs_file_seek(&lfs, &file, +10, LFS_SEEK_CUR), LFS_ERR_INVAL);

    // File pointer should not have changed
    ASSERT_EQ(lfs_file_tell(&lfs, &file), LFS_FILE_MAX);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, SeekTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
