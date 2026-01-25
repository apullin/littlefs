/*
 * Block device test cases - shared between gtest and ztest
 *
 * These tests validate that the underlying block device is working correctly.
 * Note: We use 251, a prime, to avoid aliasing powers of 2.
 *
 * USAGE: This file is #included by the test runner (gtest or ztest)
 * after defining appropriate assertion macros.
 */

#include "test/cases/test_bd.h"
#include <string.h>

/* These macros must be defined by the including file:
 * - LFS_TEST_ASSERT_OK(expr)  - assert expr == 0
 * - LFS_TEST_ASSERT_EQ(a, b)  - assert a == b
 */

#ifndef LFS_TEST_ASSERT_OK
#error "LFS_TEST_ASSERT_OK must be defined before including test_bd.c"
#endif

#ifndef LFS_TEST_ASSERT_EQ
#error "LFS_TEST_ASSERT_EQ must be defined before including test_bd.c"
#endif

void test_bd_one_block(struct lfs_config *cfg,
                       lfs_size_t read_size,
                       lfs_size_t prog_size) {
    lfs_size_t buffer_size = (read_size > prog_size) ? read_size : prog_size;
    uint8_t buffer[buffer_size];

    /* Write data */
    LFS_TEST_ASSERT_OK(cfg->erase(cfg, 0));
    for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (i + j) % 251;
        }
        LFS_TEST_ASSERT_OK(cfg->prog(cfg, 0, i, buffer, prog_size));
    }

    /* Read data */
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, 0, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((i + j) % 251));
        }
    }
}

void test_bd_two_block(struct lfs_config *cfg,
                       lfs_size_t read_size,
                       lfs_size_t prog_size) {
    lfs_size_t buffer_size = (read_size > prog_size) ? read_size : prog_size;
    uint8_t buffer[buffer_size];
    lfs_block_t block;

    /* Write block 0 */
    block = 0;
    LFS_TEST_ASSERT_OK(cfg->erase(cfg, block));
    for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_TEST_ASSERT_OK(cfg->prog(cfg, block, i, buffer, prog_size));
    }

    /* Read block 0 */
    block = 0;
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
        }
    }

    /* Write block 1 */
    block = 1;
    LFS_TEST_ASSERT_OK(cfg->erase(cfg, block));
    for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_TEST_ASSERT_OK(cfg->prog(cfg, block, i, buffer, prog_size));
    }

    /* Read block 1 */
    block = 1;
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
        }
    }

    /* Read block 0 again - verify it wasn't corrupted */
    block = 0;
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
        }
    }
}

void test_bd_last_block(struct lfs_config *cfg,
                        lfs_size_t read_size,
                        lfs_size_t prog_size) {
    lfs_size_t buffer_size = (read_size > prog_size) ? read_size : prog_size;
    uint8_t buffer[buffer_size];
    lfs_block_t block;

    /* Write block 0 */
    block = 0;
    LFS_TEST_ASSERT_OK(cfg->erase(cfg, block));
    for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_TEST_ASSERT_OK(cfg->prog(cfg, block, i, buffer, prog_size));
    }

    /* Read block 0 */
    block = 0;
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
        }
    }

    /* Write block n-1 */
    block = cfg->block_count - 1;
    LFS_TEST_ASSERT_OK(cfg->erase(cfg, block));
    for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
        for (lfs_off_t j = 0; j < prog_size; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_TEST_ASSERT_OK(cfg->prog(cfg, block, i, buffer, prog_size));
    }

    /* Read block n-1 */
    block = cfg->block_count - 1;
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
        }
    }

    /* Read block 0 again */
    block = 0;
    for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
        LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
        for (lfs_off_t j = 0; j < read_size; j++) {
            LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
        }
    }
}

void test_bd_powers_of_two(struct lfs_config *cfg,
                           lfs_size_t read_size,
                           lfs_size_t prog_size) {
    lfs_size_t buffer_size = (read_size > prog_size) ? read_size : prog_size;
    uint8_t buffer[buffer_size];

    /* Write/read every power of 2 */
    lfs_block_t block = 1;
    while (block < cfg->block_count) {
        /* Write */
        LFS_TEST_ASSERT_OK(cfg->erase(cfg, block));
        for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
            for (lfs_off_t j = 0; j < prog_size; j++) {
                buffer[j] = (block + i + j) % 251;
            }
            LFS_TEST_ASSERT_OK(cfg->prog(cfg, block, i, buffer, prog_size));
        }

        /* Read */
        for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
            LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
            for (lfs_off_t j = 0; j < read_size; j++) {
                LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
            }
        }

        block *= 2;
    }

    /* Read every power of 2 again */
    block = 1;
    while (block < cfg->block_count) {
        for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
            LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
            for (lfs_off_t j = 0; j < read_size; j++) {
                LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
            }
        }
        block *= 2;
    }
}

void test_bd_fibonacci(struct lfs_config *cfg,
                       lfs_size_t read_size,
                       lfs_size_t prog_size) {
    lfs_size_t buffer_size = (read_size > prog_size) ? read_size : prog_size;
    uint8_t buffer[buffer_size];

    /* Write/read every Fibonacci number on our device */
    lfs_block_t block = 1;
    lfs_block_t block_prev = 1;
    while (block < cfg->block_count) {
        /* Write */
        LFS_TEST_ASSERT_OK(cfg->erase(cfg, block));
        for (lfs_off_t i = 0; i < cfg->block_size; i += prog_size) {
            for (lfs_off_t j = 0; j < prog_size; j++) {
                buffer[j] = (block + i + j) % 251;
            }
            LFS_TEST_ASSERT_OK(cfg->prog(cfg, block, i, buffer, prog_size));
        }

        /* Read */
        for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
            LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
            for (lfs_off_t j = 0; j < read_size; j++) {
                LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
            }
        }

        lfs_block_t next_block = block + block_prev;
        block_prev = block;
        block = next_block;
    }

    /* Read every Fibonacci number again */
    block = 1;
    block_prev = 1;
    while (block < cfg->block_count) {
        for (lfs_off_t i = 0; i < cfg->block_size; i += read_size) {
            LFS_TEST_ASSERT_OK(cfg->read(cfg, block, i, buffer, read_size));
            for (lfs_off_t j = 0; j < read_size; j++) {
                LFS_TEST_ASSERT_EQ(buffer[j], (uint8_t)((block + i + j) % 251));
            }
        }

        lfs_block_t next_block = block + block_prev;
        block_prev = block;
        block = next_block;
    }
}
