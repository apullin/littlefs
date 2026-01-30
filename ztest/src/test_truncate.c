/*
 * File truncate tests
 */
#include "lfs_ztest_common.h"

static void *truncate_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void truncate_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(truncate_tests, NULL, truncate_setup, truncate_before, NULL, NULL);

/* Simple truncate */
ZTEST(truncate_tests, test_truncate_simple)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "truncme",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "0123456789", 10), 10,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Truncate to smaller size */
    zassert_equal(lfs_file_open(&lfs, &file, "truncme", LFS_O_RDWR), 0,
                  "Open RW failed");
    zassert_equal(lfs_file_truncate(&lfs, &file, 5), 0, "Truncate failed");
    zassert_equal(lfs_file_size(&lfs, &file), 5, "Size mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "truncme", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 16), 5, "Read failed");
    zassert_equal(memcmp(buffer, "01234", 5), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Truncate to zero */
ZTEST(truncate_tests, test_truncate_to_zero)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "trunczero",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "0123456789", 10), 10,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Truncate to zero */
    zassert_equal(lfs_file_open(&lfs, &file, "trunczero", LFS_O_RDWR), 0,
                  "Open RW failed");
    zassert_equal(lfs_file_truncate(&lfs, &file, 0), 0, "Truncate failed");
    zassert_equal(lfs_file_size(&lfs, &file), 0, "Size mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "trunczero", &info), 0, "Stat failed");
    zassert_equal(info.size, 0, "Size not zero");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Truncate to extend */
ZTEST(truncate_tests, test_truncate_extend)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "truncextend",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "hello", 5), 5, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Extend */
    zassert_equal(lfs_file_open(&lfs, &file, "truncextend", LFS_O_RDWR), 0,
                  "Open RW failed");
    zassert_equal(lfs_file_truncate(&lfs, &file, 10), 0, "Truncate failed");
    zassert_equal(lfs_file_size(&lfs, &file), 10, "Size mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify - original data followed by zeros */
    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "truncextend", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 16), 10, "Read failed");
    zassert_equal(memcmp(buffer, "hello", 5), 0, "Data mismatch");
    /* Extended area should be zeros */
    for (int i = 5; i < 10; i++) {
        zassert_equal(buffer[i], 0, "Extended area not zero at %d", i);
    }
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
