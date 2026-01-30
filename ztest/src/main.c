/*
 * littlefs ztest - Block device tests
 *
 * These tests validate that the underlying block device is working correctly.
 * Note: We use 251, a prime, to avoid aliasing powers of 2.
 */
#define LFS_TEST_ZTEST

#include <zephyr/ztest.h>
#include <stdbool.h>
#include <string.h>

#include "lfs.h"
#include "test/lfs_test_bd.h"

/* Shared globals for all test suites */
struct lfs_test_bd ztest_bd;
struct lfs_config ztest_cfg;
bool ztest_bd_initialized = false;

/* Local aliases for bd_tests */
#define bd ztest_bd
#define cfg ztest_cfg

static void *bd_suite_setup(void)
{
    int ret = lfs_test_bd_create(&bd, &cfg, &lfs_test_geometries[0]);
    zassert_equal(ret, 0, "Failed to create block device");
    return NULL;
}

static void bd_suite_teardown(void *f)
{
    ARG_UNUSED(f);
    lfs_test_bd_destroy(&bd, &cfg);
}

static void bd_before_each(void *f)
{
    ARG_UNUSED(f);
    /* Erase entire device before each test for clean state */
    for (lfs_block_t i = 0; i < cfg.block_count; i++) {
        int ret = cfg.erase(&cfg, i);
        zassert_equal(ret, 0, "Failed to erase block %u", i);
    }
}

ZTEST_SUITE(bd_tests, NULL, bd_suite_setup, bd_before_each, NULL, bd_suite_teardown);

/*
 * Test: Write and read a single block
 */
ZTEST(bd_tests, test_one_block)
{
    lfs_size_t read_size = cfg.read_size;
    lfs_size_t prog_size = cfg.prog_size;
    uint8_t buffer[prog_size > read_size ? prog_size : read_size];

    /* Write data */
    zassert_equal(cfg.erase(&cfg, 0), 0, "Erase failed");
    for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (i + j) % 251;
        }
        zassert_equal(cfg.prog(&cfg, 0, i, buffer, prog_size), 0,
                      "Prog failed at offset %u", i);
    }

    /* Read data */
    for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
        zassert_equal(cfg.read(&cfg, 0, i, buffer, read_size), 0,
                      "Read failed at offset %u", i);
        for (lfs_off_t j = 0; j < read_size; j++) {
            zassert_equal(buffer[j], (i + j) % 251,
                          "Mismatch at offset %u", i + j);
        }
    }
}

/*
 * Test: Write and read two blocks, verify no interference
 */
ZTEST(bd_tests, test_two_block)
{
    lfs_size_t read_size = cfg.read_size;
    lfs_size_t prog_size = cfg.prog_size;
    uint8_t buffer[prog_size > read_size ? prog_size : read_size];
    lfs_block_t block;

    /* Write block 0 */
    block = 0;
    zassert_equal(cfg.erase(&cfg, block), 0, "Erase failed");
    for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        zassert_equal(cfg.prog(&cfg, block, i, buffer, prog_size), 0,
                      "Prog failed");
    }

    /* Read block 0 */
    for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
        zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                      "Read failed");
        for (lfs_off_t j = 0; j < read_size; j++) {
            zassert_equal(buffer[j], (block + i + j) % 251,
                          "Mismatch at offset %u", i + j);
        }
    }

    /* Write block 1 */
    block = 1;
    zassert_equal(cfg.erase(&cfg, block), 0, "Erase failed");
    for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        zassert_equal(cfg.prog(&cfg, block, i, buffer, prog_size), 0,
                      "Prog failed");
    }

    /* Read block 1 */
    for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
        zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                      "Read failed");
        for (lfs_off_t j = 0; j < read_size; j++) {
            zassert_equal(buffer[j], (block + i + j) % 251,
                          "Mismatch at offset %u", i + j);
        }
    }

    /* Read block 0 again - verify not corrupted */
    block = 0;
    for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
        zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                      "Read failed");
        for (lfs_off_t j = 0; j < read_size; j++) {
            zassert_equal(buffer[j], (block + i + j) % 251,
                          "Block 0 was corrupted");
        }
    }
}

/*
 * Test: Write first and last blocks
 */
ZTEST(bd_tests, test_last_block)
{
    lfs_size_t read_size = cfg.read_size;
    lfs_size_t prog_size = cfg.prog_size;
    uint8_t buffer[prog_size > read_size ? prog_size : read_size];
    lfs_block_t block;

    /* Write block 0 */
    block = 0;
    zassert_equal(cfg.erase(&cfg, block), 0, "Erase failed");
    for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        zassert_equal(cfg.prog(&cfg, block, i, buffer, prog_size), 0,
                      "Prog failed");
    }

    /* Write block n-1 */
    block = cfg.block_count - 1;
    zassert_equal(cfg.erase(&cfg, block), 0, "Erase failed");
    for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        zassert_equal(cfg.prog(&cfg, block, i, buffer, prog_size), 0,
                      "Prog failed");
    }

    /* Read block 0 */
    block = 0;
    for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
        zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                      "Read failed");
        for (lfs_off_t j = 0; j < read_size; j++) {
            zassert_equal(buffer[j], (block + i + j) % 251,
                          "Mismatch");
        }
    }

    /* Read block n-1 */
    block = cfg.block_count - 1;
    for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
        zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                      "Read failed");
        for (lfs_off_t j = 0; j < read_size; j++) {
            zassert_equal(buffer[j], (block + i + j) % 251,
                          "Mismatch");
        }
    }
}

/*
 * Test: Write and read blocks at powers of two
 */
ZTEST(bd_tests, test_powers_of_two)
{
    lfs_size_t read_size = cfg.read_size;
    lfs_size_t prog_size = cfg.prog_size;
    uint8_t buffer[prog_size > read_size ? prog_size : read_size];

    /* Write/read every power of 2 */
    lfs_block_t block = 1;
    while (block < cfg.block_count) {
        /* Write */
        zassert_equal(cfg.erase(&cfg, block), 0, "Erase failed");
        for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
            for (lfs_off_t j = 0; j < prog_size; j++) {
                buffer[j] = (block + i + j) % 251;
            }
            zassert_equal(cfg.prog(&cfg, block, i, buffer, prog_size), 0,
                          "Prog failed");
        }

        /* Read */
        for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
            zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                          "Read failed");
            for (lfs_off_t j = 0; j < read_size; j++) {
                zassert_equal(buffer[j], (block + i + j) % 251,
                              "Mismatch at block %u", block);
            }
        }

        block *= 2;
    }
}

/*
 * Test: Write and read blocks at Fibonacci numbers
 */
ZTEST(bd_tests, test_fibonacci)
{
    lfs_size_t read_size = cfg.read_size;
    lfs_size_t prog_size = cfg.prog_size;
    uint8_t buffer[prog_size > read_size ? prog_size : read_size];

    /* Write/read every Fibonacci number */
    lfs_block_t block = 1;
    lfs_block_t block_prev = 1;
    while (block < cfg.block_count) {
        /* Write */
        zassert_equal(cfg.erase(&cfg, block), 0, "Erase failed");
        for (lfs_off_t i = 0; i < cfg.block_size; i += prog_size) {
            for (lfs_off_t j = 0; j < prog_size; j++) {
                buffer[j] = (block + i + j) % 251;
            }
            zassert_equal(cfg.prog(&cfg, block, i, buffer, prog_size), 0,
                          "Prog failed");
        }

        /* Read */
        for (lfs_off_t i = 0; i < cfg.block_size; i += read_size) {
            zassert_equal(cfg.read(&cfg, block, i, buffer, read_size), 0,
                          "Read failed");
            for (lfs_off_t j = 0; j < read_size; j++) {
                zassert_equal(buffer[j], (block + i + j) % 251,
                              "Mismatch at block %u", block);
            }
        }

        lfs_block_t next = block + block_prev;
        block_prev = block;
        block = next;
    }
}
