/*
 * File operation tests - read/write/append/truncate
 */
#include "lfs_ztest_common.h"
#include <stdio.h>

static void *files_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void files_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(files_tests, NULL, files_setup, files_before, NULL, NULL);

/* Simple file write and read */
ZTEST(files_tests, test_simple_file)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "hello",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");

    const char *message = "Hello World!";
    lfs_size_t size = strlen(message) + 1;
    uint8_t buffer[64];
    strcpy((char *)buffer, message);
    zassert_equal(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Remount and read */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    zassert_equal(lfs_file_open(&lfs, &file, "hello", LFS_O_RDONLY), 0,
                  "Reopen failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size,
                  "Read failed");
    zassert_equal(strcmp((char *)buffer, message), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Large file write and read */
ZTEST(files_tests, test_large_file)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "avocado",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");

    /* Write 8KB of data in chunks */
    lfs_size_t total_size = 8192;
    lfs_size_t chunk_size = 64;
    uint8_t buffer[64];
    uint32_t prng = 1;

    for (lfs_size_t i = 0; i < total_size; i += chunk_size) {
        lfs_size_t chunk = (total_size - i < chunk_size) ? (total_size - i) : chunk_size;
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = lfs_ztest_prng(&prng) & 0xff;
        }
        zassert_equal(lfs_file_write(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk,
                      "Write failed at %u", i);
    }
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Read back and verify */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    zassert_equal(lfs_file_open(&lfs, &file, "avocado", LFS_O_RDONLY), 0,
                  "Reopen failed");
    zassert_equal(lfs_file_size(&lfs, &file), (lfs_soff_t)total_size,
                  "Size mismatch");

    prng = 1;
    for (lfs_size_t i = 0; i < total_size; i += chunk_size) {
        lfs_size_t chunk = (total_size - i < chunk_size) ? (total_size - i) : chunk_size;
        zassert_equal(lfs_file_read(&lfs, &file, buffer, chunk), (lfs_ssize_t)chunk,
                      "Read failed at %u", i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            uint8_t expected = lfs_ztest_prng(&prng) & 0xff;
            zassert_equal(buffer[b], expected, "Data mismatch at %u", i + b);
        }
    }
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* File append test */
ZTEST(files_tests, test_append)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    /* Initial write */
    zassert_equal(lfs_file_open(&lfs, &file, "appendme",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "hello", 5), 5, "Write1 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Append */
    zassert_equal(lfs_file_open(&lfs, &file, "appendme",
            LFS_O_WRONLY | LFS_O_APPEND), 0, "Open append failed");
    zassert_equal(lfs_file_write(&lfs, &file, "world", 5), 5, "Write2 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Read and verify */
    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "appendme", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_size(&lfs, &file), 10, "Size mismatch");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 10), 10, "Read failed");
    zassert_equal(memcmp(buffer, "helloworld", 10), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* File truncate test */
ZTEST(files_tests, test_truncate)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    /* Write initial data */
    zassert_equal(lfs_file_open(&lfs, &file, "truncme",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "hello world!", 12), 12,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Truncate */
    zassert_equal(lfs_file_open(&lfs, &file, "truncme",
            LFS_O_WRONLY | LFS_O_TRUNC), 0, "Open trunc failed");
    zassert_equal(lfs_file_write(&lfs, &file, "hi", 2), 2, "Write2 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "truncme", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_size(&lfs, &file), 2, "Size mismatch");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 16), 2, "Read failed");
    zassert_equal(memcmp(buffer, "hi", 2), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Many files test */
ZTEST(files_tests, test_many_files)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Create multiple files */
    const int N = 10;
    for (int i = 0; i < N; i++) {
        char name[32];
        snprintf(name, sizeof(name), "file%03d", i);
        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, name,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0,
                "Open %s failed", name);
        zassert_equal(lfs_file_write(&lfs, &file, name, strlen(name)), (lfs_ssize_t)strlen(name),
                      "Write %s failed", name);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %s failed", name);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Remount and verify */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    for (int i = 0; i < N; i++) {
        char name[32];
        char buffer[32];
        snprintf(name, sizeof(name), "file%03d", i);
        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, name, LFS_O_RDONLY), 0,
                      "Open %s failed", name);
        int len = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
        zassert_equal(len, (int)strlen(name), "Read %s size mismatch", name);
        buffer[len] = '\0';
        zassert_equal(strcmp(buffer, name), 0, "Data mismatch for %s", name);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %s failed", name);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Rewrite file test */
ZTEST(files_tests, test_rewrite)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    /* Initial write */
    zassert_equal(lfs_file_open(&lfs, &file, "rewrite",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "original", 8), 8, "Write1 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Rewrite with different data */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    zassert_equal(lfs_file_open(&lfs, &file, "rewrite", LFS_O_WRONLY), 0,
                  "Open rewrite failed");
    zassert_equal(lfs_file_write(&lfs, &file, "replaced", 8), 8, "Write2 failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Verify */
    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "rewrite", LFS_O_RDONLY), 0,
                  "Open read failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 8), 8, "Read failed");
    zassert_equal(memcmp(buffer, "replaced", 8), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
