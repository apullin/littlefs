/*
 * Common test infrastructure for littlefs ztest
 */
#ifndef LFS_ZTEST_COMMON_H
#define LFS_ZTEST_COMMON_H

#include <zephyr/ztest.h>
#include <string.h>

#include "lfs.h"
#include "test/lfs_test_bd.h"

/* Shared block device and config */
extern struct lfs_test_bd ztest_bd;
extern struct lfs_config ztest_cfg;
extern bool ztest_bd_initialized;

/* Initialize the block device if not already done */
static inline void lfs_ztest_init_device(void)
{
    if (!ztest_bd_initialized) {
        int ret = lfs_test_bd_create(&ztest_bd, &ztest_cfg, &lfs_test_geometries[0]);
        zassert_equal(ret, 0, "Failed to create block device");
        ztest_bd_initialized = true;
    }
}

/* Helper to erase the filesystem before test */
static inline void lfs_ztest_format_device(void)
{
    lfs_ztest_init_device();
    /* Erase entire device for clean state */
    for (lfs_block_t i = 0; i < ztest_cfg.block_count; i++) {
        int ret = ztest_cfg.erase(&ztest_cfg, i);
        zassert_equal(ret, 0, "Failed to erase block %u", i);
    }
}

/* Simple PRNG for test data generation */
static inline uint32_t lfs_ztest_prng(uint32_t *state)
{
    *state = *state * 1103515245 + 12345;
    return *state;
}

#endif /* LFS_ZTEST_COMMON_H */
