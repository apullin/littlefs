/*
 * Block device abstraction for littlefs tests
 * Allows same test logic to run with different BD implementations
 */
#ifndef LFS_TEST_BD_H
#define LFS_TEST_BD_H

#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Geometry configuration for parametric tests */
struct lfs_test_geometry {
    const char *name;
    lfs_size_t read_size;
    lfs_size_t prog_size;
    lfs_size_t erase_size;
    lfs_size_t erase_count;
};

/* Standard geometries for testing */
extern const struct lfs_test_geometry lfs_test_geometries[];
extern const size_t lfs_test_geometry_count;

/*
 * Block device context - implementation-specific
 * For emubd: wraps lfs_emubd_t and lfs_emubd_config
 * For zephyr: wraps flash device and offset
 */
#if defined(LFS_TEST_ZTEST)
#include <zephyr/device.h>

struct lfs_test_bd {
    const struct device *flash_dev;
    size_t offset;
    size_t size;
};
#else
/* Forward declaration for gtest - defined in lfs_test_bd_emubd.c */
struct lfs_test_bd;
#endif

/*
 * Initialize block device and lfs_config for testing
 *
 * @param bd      Block device context to initialize
 * @param cfg     lfs_config to populate with callbacks
 * @param geom    Geometry configuration
 * @return 0 on success, negative error code on failure
 */
int lfs_test_bd_create(struct lfs_test_bd *bd,
                       struct lfs_config *cfg,
                       const struct lfs_test_geometry *geom);

/*
 * Cleanup block device
 */
int lfs_test_bd_destroy(struct lfs_test_bd *bd,
                        struct lfs_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* LFS_TEST_BD_H */
