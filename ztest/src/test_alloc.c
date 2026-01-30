/*
 * Block allocation tests
 */
#include "lfs_ztest_common.h"
#include <stdio.h>

static void *alloc_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void alloc_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(alloc_tests, NULL, alloc_setup, alloc_before, NULL, NULL);

/* Parallel allocation - create many files without closing */
ZTEST(alloc_tests, test_parallel_alloc)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 3;  /* Limited by open file handles */
    lfs_file_t files[3];

    /* Open multiple files at once */
    for (int i = 0; i < N; i++) {
        char name[32];
        snprintf(name, sizeof(name), "parallel%d", i);
        zassert_equal(lfs_file_open(&lfs, &files[i], name,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0,
                "Open %d failed", i);
    }

    /* Write to all of them */
    for (int i = 0; i < N; i++) {
        char data[32];
        snprintf(data, sizeof(data), "data%d", i);
        zassert_equal(lfs_file_write(&lfs, &files[i], data, strlen(data)),
                      (lfs_ssize_t)strlen(data), "Write %d failed", i);
    }

    /* Close all */
    for (int i = 0; i < N; i++) {
        zassert_equal(lfs_file_close(&lfs, &files[i]), 0, "Close %d failed", i);
    }

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    for (int i = 0; i < N; i++) {
        char name[32], expected[32], buffer[32];
        snprintf(name, sizeof(name), "parallel%d", i);
        snprintf(expected, sizeof(expected), "data%d", i);
        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, name, LFS_O_RDONLY), 0,
                      "Reopen %d failed", i);
        int len = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
        zassert_equal(len, (int)strlen(expected), "Read %d size wrong", i);
        buffer[len] = '\0';
        zassert_equal(strcmp(buffer, expected), 0, "Data %d mismatch", i);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %d failed", i);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Serial allocation - create and close files sequentially */
ZTEST(alloc_tests, test_serial_alloc)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 10;
    for (int i = 0; i < N; i++) {
        char name[32], data[32];
        snprintf(name, sizeof(name), "serial%d", i);
        snprintf(data, sizeof(data), "serial_data_%d", i);

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
        char name[32], expected[32], buffer[64];
        snprintf(name, sizeof(name), "serial%d", i);
        snprintf(expected, sizeof(expected), "serial_data_%d", i);

        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, name, LFS_O_RDONLY), 0,
                      "Reopen %d failed", i);
        int len = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
        zassert_equal(len, (int)strlen(expected), "Read %d size wrong", i);
        buffer[len] = '\0';
        zassert_equal(strcmp(buffer, expected), 0, "Data %d mismatch", i);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %d failed", i);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/*
 * NOTE: Exhaustion test removed - the Zephyr flash simulator hits internal
 * assertions when trying to fill the filesystem completely. This test
 * requires the emubd which tracks wear properly. Test coverage for
 * exhaustion is provided by gtest with emubd.
 */
