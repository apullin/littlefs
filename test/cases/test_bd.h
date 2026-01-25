/*
 * Block device test cases - shared between gtest and ztest
 *
 * These tests validate that the underlying block device is working correctly.
 * Note: We use 251, a prime, to avoid aliasing powers of 2.
 */
#ifndef LFS_TEST_CASES_BD_H
#define LFS_TEST_CASES_BD_H

#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Test: Write and read a single block
 * Validates basic erase/prog/read operations
 */
void test_bd_one_block(struct lfs_config *cfg,
                       lfs_size_t read_size,
                       lfs_size_t prog_size);

/*
 * Test: Write and read two blocks, verify no interference
 */
void test_bd_two_block(struct lfs_config *cfg,
                       lfs_size_t read_size,
                       lfs_size_t prog_size);

/*
 * Test: Write first and last blocks
 */
void test_bd_last_block(struct lfs_config *cfg,
                        lfs_size_t read_size,
                        lfs_size_t prog_size);

/*
 * Test: Write and read blocks at powers of two
 */
void test_bd_powers_of_two(struct lfs_config *cfg,
                           lfs_size_t read_size,
                           lfs_size_t prog_size);

/*
 * Test: Write and read blocks at Fibonacci numbers
 */
void test_bd_fibonacci(struct lfs_config *cfg,
                       lfs_size_t read_size,
                       lfs_size_t prog_size);

#ifdef __cplusplus
}
#endif

#endif /* LFS_TEST_CASES_BD_H */
