/*
 * File truncate tests
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class TruncateTest : public LfsParametricTest {};

// Simple truncate
TEST_P(TruncateTest, Simple) {
    const lfs_size_t MEDIUMSIZE = 512;
    const lfs_size_t LARGESIZE = 2048;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify truncated size
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Truncate to zero
TEST_P(TruncateTest, ToZero) {
    const lfs_size_t LARGESIZE = 1024;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldy",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate to zero
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldy", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, 0), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify empty
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldy", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Extend via truncate
TEST_P(TruncateTest, Extend) {
    const lfs_size_t SMALLSIZE = 32;
    const lfs_size_t LARGESIZE = 512;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "grow",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    const char* data = "seed";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < SMALLSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, SMALLSIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Extend via truncate
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "grow", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)SMALLSIZE);
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, LARGESIZE), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify extended data (zeros after original)
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "grow", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);

    uint8_t buffer[1024];
    for (lfs_off_t j = 0; j < SMALLSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, SMALLSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    // Rest should be zeros
    lfs_size_t remaining = LARGESIZE - SMALLSIZE;
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, remaining), (lfs_ssize_t)remaining);
    for (lfs_size_t i = 0; i < remaining; i++) {
        ASSERT_EQ(buffer[i], 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Truncate and read in same session
TEST_P(TruncateTest, Read) {
    const lfs_size_t MEDIUMSIZE = 512;
    const lfs_size_t LARGESIZE = 2048;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldyread",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate and read in same session
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldyread", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);

    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    // Read immediately after truncate
    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify persistence
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldyread", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Write, truncate, and read
TEST_P(TruncateTest, WriteRead) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "sequence",
            LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC), 0);

    uint8_t buffer[1024];
    size_t size = lfs_min((size_t)cfg_.cache_size, sizeof(buffer)/2);
    lfs_size_t qsize = size / 4;
    uint8_t *wb = buffer;
    uint8_t *rb = buffer + size;
    for (lfs_off_t j = 0; j < (lfs_off_t)size; ++j) {
        wb[j] = j;
    }

    // Spread sequence over size
    ASSERT_EQ(lfs_file_write(&lfs, &file, wb, size), (lfs_ssize_t)size);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)size);
    ASSERT_EQ(lfs_file_tell(&lfs, &file), (lfs_soff_t)size);

    ASSERT_EQ(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET), 0);
    ASSERT_EQ(lfs_file_tell(&lfs, &file), 0);

    // Chop off the last quarter
    lfs_size_t trunc = size - qsize;
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, trunc), 0);
    ASSERT_EQ(lfs_file_tell(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)trunc);

    // Read should produce first 3/4
    ASSERT_EQ(lfs_file_read(&lfs, &file, rb, size), (lfs_ssize_t)trunc);
    ASSERT_EQ(memcmp(rb, wb, trunc), 0);

    // Move to 1/4
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)trunc);
    ASSERT_EQ(lfs_file_seek(&lfs, &file, qsize, LFS_SEEK_SET), (lfs_soff_t)qsize);
    ASSERT_EQ(lfs_file_tell(&lfs, &file), (lfs_soff_t)qsize);

    // Chop to 1/2
    trunc -= qsize;
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, trunc), 0);
    ASSERT_EQ(lfs_file_tell(&lfs, &file), (lfs_soff_t)qsize);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)trunc);

    // Read should produce second quarter
    ASSERT_EQ(lfs_file_read(&lfs, &file, rb, size), (lfs_ssize_t)(trunc - qsize));
    ASSERT_EQ(memcmp(rb, wb + qsize, trunc - qsize), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Truncate and write
TEST_P(TruncateTest, Write) {
    const lfs_size_t MEDIUMSIZE = 512;
    const lfs_size_t LARGESIZE = 2048;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldywrite",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate and write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldywrite", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)LARGESIZE);

    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    // Write new data
    const char* newdata = "bald";
    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, newdata, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify new data
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldywrite", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, newdata, chunk), 0);
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Noop truncate (truncate to same size)
TEST_P(TruncateTest, Noop) {
    const lfs_size_t MEDIUMSIZE = 512;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop",
            LFS_O_RDWR | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);

        // This truncate should do nothing
        ASSERT_EQ(lfs_file_truncate(&lfs, &file, j + chunk), 0);
    }
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    ASSERT_EQ(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET), 0);
    // Should do nothing again
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);

    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);

    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Still there after reboot?
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);
    for (lfs_off_t j = 0; j < MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Aggressive truncation tests - cold shrinking
TEST_P(TruncateTest, AggressiveColdShrink) {
    const lfs_size_t SMALLSIZE = 32;
    const lfs_size_t MEDIUMSIZE = 2048;
    const lfs_size_t LARGESIZE = 8192;
    const int COUNT = 5;
    const lfs_off_t startsizes[COUNT] = {2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE};
    const lfs_off_t hotsizes[COUNT] = {2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE};
    const lfs_off_t coldsizes[COUNT] = {0, SMALLSIZE, MEDIUMSIZE, LARGESIZE, 2*LARGESIZE};

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create files with initial sizes
    for (int i = 0; i < COUNT; i++) {
        char path[32];
        snprintf(path, sizeof(path), "hairyhead%d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);

        uint8_t buffer[1024];
        const char* data = "hair";
        size_t size = strlen(data);
        for (lfs_off_t j = 0; j < startsizes[i]; j += size) {
            ASSERT_EQ(lfs_file_write(&lfs, &file, data, size), (lfs_ssize_t)size);
        }
        ASSERT_EQ(lfs_file_size(&lfs, &file), startsizes[i]);

        ASSERT_EQ(lfs_file_truncate(&lfs, &file, hotsizes[i]), 0);
        ASSERT_EQ(lfs_file_size(&lfs, &file), hotsizes[i]);

        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Cold truncate
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < COUNT; i++) {
        char path[32];
        snprintf(path, sizeof(path), "hairyhead%d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDWR), 0);
        ASSERT_EQ(lfs_file_size(&lfs, &file), hotsizes[i]);

        ASSERT_EQ(lfs_file_truncate(&lfs, &file, coldsizes[i]), 0);
        ASSERT_EQ(lfs_file_size(&lfs, &file), coldsizes[i]);

        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < COUNT; i++) {
        char path[32];
        snprintf(path, sizeof(path), "hairyhead%d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
        ASSERT_EQ(lfs_file_size(&lfs, &file), coldsizes[i]);

        uint8_t buffer[1024];
        const char* data = "hair";
        size_t size = strlen(data);
        for (lfs_off_t j = 0; j < coldsizes[i]; j += size) {
            ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
            ASSERT_EQ(memcmp(buffer, data, size), 0);
        }

        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Aggressive truncation tests - warm expanding
TEST_P(TruncateTest, AggressiveWarmExpand) {
    const lfs_size_t SMALLSIZE = 32;
    const lfs_size_t MEDIUMSIZE = 2048;
    const lfs_size_t LARGESIZE = 8192;
    const int COUNT = 5;
    const lfs_off_t startsizes[COUNT] = {0, SMALLSIZE, MEDIUMSIZE, LARGESIZE, 2*LARGESIZE};
    const lfs_off_t hotsizes[COUNT] = {2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE, 2*LARGESIZE};

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create and expand files
    for (int i = 0; i < COUNT; i++) {
        char path[32];
        snprintf(path, sizeof(path), "hairyhead%d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);

        uint8_t buffer[1024];
        const char* data = "hair";
        size_t size = strlen(data);
        for (lfs_off_t j = 0; j < startsizes[i]; j += size) {
            ASSERT_EQ(lfs_file_write(&lfs, &file, data, size), (lfs_ssize_t)size);
        }
        ASSERT_EQ(lfs_file_size(&lfs, &file), startsizes[i]);

        // Expand via truncate
        ASSERT_EQ(lfs_file_truncate(&lfs, &file, hotsizes[i]), 0);
        ASSERT_EQ(lfs_file_size(&lfs, &file), hotsizes[i]);

        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify - original data followed by zeros
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < COUNT; i++) {
        char path[32];
        snprintf(path, sizeof(path), "hairyhead%d", i);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
        ASSERT_EQ(lfs_file_size(&lfs, &file), hotsizes[i]);

        uint8_t buffer[1024];
        const char* data = "hair";
        size_t size = strlen(data);
        lfs_off_t j = 0;
        for (; j < startsizes[i]; j += size) {
            ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
            ASSERT_EQ(memcmp(buffer, data, size), 0);
        }
        // Rest should be zeros
        for (; j < hotsizes[i]; j += size) {
            ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
            ASSERT_EQ(memcmp(buffer, "\0\0\0\0", size), 0);
        }

        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Parameterized truncate test with MEDIUMSIZE/LARGESIZE combinations
struct TruncateSizeParams {
    LfsGeometry geometry;
    lfs_size_t medium_size;
    lfs_size_t large_size;

    std::string Name() const {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s_med%u_lg%u",
                geometry.name, (unsigned)medium_size, (unsigned)large_size);
        return buf;
    }
};

class TruncateSizeTest : public LfsTestFixture,
                         public ::testing::WithParamInterface<TruncateSizeParams> {
protected:
    void SetUp() override {
        SetGeometry(GetParam().geometry);
        LfsTestFixture::SetUp();
    }
};

struct TruncateSizeNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<TruncateSizeParams>& info) const {
        return info.param.Name();
    }
};

std::vector<TruncateSizeParams> GenerateTruncateSizeParams() {
    std::vector<TruncateSizeParams> params;
    auto geometries = AllGeometries();
    lfs_size_t mediums[] = {32, 33, 512, 513, 2048, 2049};
    lfs_size_t larges[] = {512, 513, 2048, 2049, 8192, 8193};

    for (const auto &geom : geometries) {
        for (auto med : mediums) {
            for (auto lg : larges) {
                if (med < lg) {
                    params.push_back({geom, med, lg});
                }
            }
        }
    }
    return params;
}

TEST_P(TruncateSizeTest, SimpleSizeCombo) {
    lfs_size_t MEDIUMSIZE = GetParam().medium_size;
    lfs_size_t LARGESIZE = GetParam().large_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < (lfs_off_t)LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate to medium
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldynoop", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);
    for (lfs_off_t j = 0; j < (lfs_off_t)MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

TEST_P(TruncateSizeTest, ReadSizeCombo) {
    lfs_size_t MEDIUMSIZE = GetParam().medium_size;
    lfs_size_t LARGESIZE = GetParam().large_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldyread",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < (lfs_off_t)LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate and read in same session
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldyread", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);

    for (lfs_off_t j = 0; j < (lfs_off_t)MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, data, chunk), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

TEST_P(TruncateSizeTest, WriteSizeCombo) {
    lfs_size_t MEDIUMSIZE = GetParam().medium_size;
    lfs_size_t LARGESIZE = GetParam().large_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldywrite",
            LFS_O_WRONLY | LFS_O_CREAT), 0);

    uint8_t buffer[1024];
    const char* data = "hair";
    size_t size = strlen(data);
    for (lfs_off_t j = 0; j < (lfs_off_t)LARGESIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, LARGESIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, data, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate and write new data
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldywrite", LFS_O_RDWR), 0);
    ASSERT_EQ(lfs_file_truncate(&lfs, &file, MEDIUMSIZE), 0);

    const char* newdata = "bald";
    lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET);
    for (lfs_off_t j = 0; j < (lfs_off_t)MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_write(&lfs, &file, newdata, chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "baldywrite", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)MEDIUMSIZE);
    for (lfs_off_t j = 0; j < (lfs_off_t)MEDIUMSIZE; j += size) {
        lfs_size_t chunk = lfs_min(size, MEDIUMSIZE - j);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk);
        ASSERT_EQ(memcmp(buffer, newdata, chunk), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, TruncateTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});

INSTANTIATE_TEST_SUITE_P(
    TruncSizes, TruncateSizeTest,
    ::testing::ValuesIn(GenerateTruncateSizeParams()),
    TruncateSizeNameGenerator{});
