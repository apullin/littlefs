/*
 * File seek tests
 */
#include "lfs_ztest_common.h"

static void *seek_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void seek_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(seek_tests, NULL, seek_setup, seek_before, NULL, NULL);

/* Seek during read */
ZTEST(seek_tests, test_seek_read)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Write test data */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "seekme",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");

    const char *data = "0123456789ABCDEFGHIJ";
    zassert_equal(lfs_file_write(&lfs, &file, data, 20), 20, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Read with seeks */
    zassert_equal(lfs_file_open(&lfs, &file, "seekme", LFS_O_RDONLY), 0,
                  "Open read failed");

    uint8_t buffer[8];

    /* Read from start */
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 4), 4, "Read1 failed");
    zassert_equal(memcmp(buffer, "0123", 4), 0, "Data1 mismatch");

    /* Seek to position 10 */
    zassert_equal(lfs_file_seek(&lfs, &file, 10, LFS_SEEK_SET), 10,
                  "Seek1 failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 4), 4, "Read2 failed");
    zassert_equal(memcmp(buffer, "ABCD", 4), 0, "Data2 mismatch");

    /* Seek relative */
    zassert_equal(lfs_file_seek(&lfs, &file, -8, LFS_SEEK_CUR), 6,
                  "Seek2 failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 4), 4, "Read3 failed");
    zassert_equal(memcmp(buffer, "6789", 4), 0, "Data3 mismatch");

    /* Seek from end */
    zassert_equal(lfs_file_seek(&lfs, &file, -4, LFS_SEEK_END), 16,
                  "Seek3 failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 4), 4, "Read4 failed");
    zassert_equal(memcmp(buffer, "GHIJ", 4), 0, "Data4 mismatch");

    /* Tell position */
    zassert_equal(lfs_file_tell(&lfs, &file), 20, "Tell failed");

    /* Rewind */
    zassert_equal(lfs_file_rewind(&lfs, &file), 0, "Rewind failed");
    zassert_equal(lfs_file_tell(&lfs, &file), 0, "Tell after rewind failed");

    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Seek during write */
ZTEST(seek_tests, test_seek_write)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Create file with initial data */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "seekwrite",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "0123456789", 10), 10,
                  "Write1 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Reopen and seek-write */
    zassert_equal(lfs_file_open(&lfs, &file, "seekwrite", LFS_O_RDWR), 0,
                  "Open RW failed");

    /* Overwrite middle */
    zassert_equal(lfs_file_seek(&lfs, &file, 3, LFS_SEEK_SET), 3,
                  "Seek failed");
    zassert_equal(lfs_file_write(&lfs, &file, "XXX", 3), 3, "Write2 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "seekwrite", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 16), 10, "Read failed");
    zassert_equal(memcmp(buffer, "012XXX6789", 10), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Seek boundary conditions */
ZTEST(seek_tests, test_seek_boundaries)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "bounds",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "0123456789", 10), 10,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_file_open(&lfs, &file, "bounds", LFS_O_RDONLY), 0,
                  "Open read failed");

    /* Seek to end */
    zassert_equal(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_END), 10,
                  "Seek end failed");

    /* Seek past end (should work) */
    zassert_equal(lfs_file_seek(&lfs, &file, 5, LFS_SEEK_END), 15,
                  "Seek past end failed");

    /* Seek before start (should fail or clamp) */
    lfs_soff_t pos = lfs_file_seek(&lfs, &file, -20, LFS_SEEK_SET);
    zassert_true(pos < 0, "Seek before start should fail");

    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
