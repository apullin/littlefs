/*
 * Inline file entry tests
 */
#include "lfs_ztest_common.h"
#include <stdio.h>

static void *entries_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void entries_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(entries_tests, NULL, entries_setup, entries_before, NULL, NULL);

/* Test inline file growth */
ZTEST(entries_tests, test_grow)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "grow",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");

    /* Write small amounts, growing the file */
    for (int i = 0; i < 10; i++) {
        char data[8];
        snprintf(data, sizeof(data), "%d", i);
        zassert_equal(lfs_file_write(&lfs, &file, data, strlen(data)),
                      (lfs_ssize_t)strlen(data), "Write %d failed", i);
        zassert_equal(lfs_file_sync(&lfs, &file), 0, "Sync %d failed", i);
    }
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    zassert_equal(lfs_file_open(&lfs, &file, "grow", LFS_O_RDONLY), 0,
                  "Open read failed");
    char buffer[64];
    lfs_ssize_t len = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
    zassert_true(len > 0, "Read failed");
    buffer[len] = '\0';
    zassert_equal(strcmp(buffer, "0123456789"), 0, "Data mismatch: %s", buffer);
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Test inline file shrink */
ZTEST(entries_tests, test_shrink)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "shrink",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "0123456789", 10), 10,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Shrink by truncating */
    zassert_equal(lfs_file_open(&lfs, &file, "shrink", LFS_O_RDWR), 0,
                  "Open RW failed");
    zassert_equal(lfs_file_truncate(&lfs, &file, 5), 0, "Truncate failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    char buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "shrink", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, sizeof(buffer)), 5,
                  "Read failed");
    zassert_equal(memcmp(buffer, "01234", 5), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Test spilling from inline to CTZ */
ZTEST(entries_tests, test_spill)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "spill",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");

    /* Write data that will exceed inline limit and spill to blocks */
    uint8_t buffer[256];
    uint32_t prng = 42;
    lfs_size_t total = 0;
    lfs_size_t target = 2048;  /* Large enough to spill */

    while (total < target) {
        lfs_size_t chunk = (target - total < sizeof(buffer)) ?
                           (target - total) : sizeof(buffer);
        for (lfs_size_t i = 0; i < chunk; i++) {
            buffer[i] = lfs_ztest_prng(&prng) & 0xff;
        }
        zassert_equal(lfs_file_write(&lfs, &file, buffer, chunk),
                      (lfs_ssize_t)chunk, "Write failed at %u", total);
        total += chunk;
    }
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    zassert_equal(lfs_file_open(&lfs, &file, "spill", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_size(&lfs, &file), (lfs_soff_t)target, "Size wrong");

    prng = 42;
    total = 0;
    while (total < target) {
        lfs_size_t chunk = (target - total < sizeof(buffer)) ?
                           (target - total) : sizeof(buffer);
        zassert_equal(lfs_file_read(&lfs, &file, buffer, chunk),
                      (lfs_ssize_t)chunk, "Read failed at %u", total);
        for (lfs_size_t i = 0; i < chunk; i++) {
            uint8_t expected = lfs_ztest_prng(&prng) & 0xff;
            zassert_equal(buffer[i], expected, "Mismatch at %u", total + i);
        }
        total += chunk;
    }
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Test many small files (inline entries) */
ZTEST(entries_tests, test_many_inline)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 10;
    for (int i = 0; i < N; i++) {
        char name[32], data[16];
        snprintf(name, sizeof(name), "inline%d", i);
        snprintf(data, sizeof(data), "d%d", i);

        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, name,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0,
                "Open %d failed", i);
        zassert_equal(lfs_file_write(&lfs, &file, data, strlen(data)),
                      (lfs_ssize_t)strlen(data), "Write %d failed", i);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %d failed", i);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    for (int i = 0; i < N; i++) {
        char name[32], expected[16], buffer[16];
        snprintf(name, sizeof(name), "inline%d", i);
        snprintf(expected, sizeof(expected), "d%d", i);

        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, name, LFS_O_RDONLY), 0,
                      "Open %d failed", i);
        int len = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
        zassert_equal(len, (int)strlen(expected), "Read %d size wrong", i);
        buffer[len] = '\0';
        zassert_equal(strcmp(buffer, expected), 0, "Data %d mismatch", i);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %d failed", i);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
