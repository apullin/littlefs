/*
 * Block device implementation using Zephyr flash API
 */
#define LFS_TEST_ZTEST

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <string.h>

#include "lfs.h"
#include "test/lfs_test_bd.h"

/* Standard geometries - use smaller values for constrained memory
 * qemu_cortex_m3 has 256KB flash, we use a portion of it
 */
const struct lfs_test_geometry lfs_test_geometries[] = {
    /* 1KB erase size, 64 blocks = 64KB total (MPS2 flash simulator) */
    {"qemu", 1, 1, 1024, 64},
};

const size_t lfs_test_geometry_count =
    sizeof(lfs_test_geometries) / sizeof(lfs_test_geometries[0]);

static int zephyr_read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size) {
    struct lfs_test_bd *bd = c->context;
    off_t addr = bd->offset + block * c->block_size + off;

    int ret = flash_read(bd->flash_dev, addr, buffer, size);
    return ret < 0 ? LFS_ERR_IO : 0;
}

static int zephyr_prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size) {
    struct lfs_test_bd *bd = c->context;
    off_t addr = bd->offset + block * c->block_size + off;

    int ret = flash_write(bd->flash_dev, addr, buffer, size);
    return ret < 0 ? LFS_ERR_IO : 0;
}

static int zephyr_erase(const struct lfs_config *c, lfs_block_t block) {
    struct lfs_test_bd *bd = c->context;
    off_t addr = bd->offset + block * c->block_size;

    int ret = flash_erase(bd->flash_dev, addr, c->block_size);
    return ret < 0 ? LFS_ERR_IO : 0;
}

static int zephyr_sync(const struct lfs_config *c) {
    ARG_UNUSED(c);
    return 0;
}

int lfs_test_bd_create(struct lfs_test_bd *bd,
                       struct lfs_config *cfg,
                       const struct lfs_test_geometry *geom) {
    memset(bd, 0, sizeof(*bd));

    /* Use simulated flash controller device */
    bd->flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    bd->offset = 0;  /* Simulated flash - full device available */
    bd->size = geom->erase_size * geom->erase_count;

    if (!device_is_ready(bd->flash_dev)) {
        return LFS_ERR_IO;
    }

    /* Initialize lfs config */
    memset(cfg, 0, sizeof(*cfg));
    cfg->context = bd;
    cfg->read = zephyr_read;
    cfg->prog = zephyr_prog;
    cfg->erase = zephyr_erase;
    cfg->sync = zephyr_sync;
    cfg->read_size = geom->read_size;
    cfg->prog_size = geom->prog_size;
    cfg->block_size = geom->erase_size;
    cfg->block_count = geom->erase_count;
    cfg->block_cycles = -1;
    cfg->cache_size = 64;
    cfg->lookahead_size = 16;

    return 0;
}

int lfs_test_bd_destroy(struct lfs_test_bd *bd,
                        struct lfs_config *cfg) {
    ARG_UNUSED(bd);
    ARG_UNUSED(cfg);
    return 0;
}
