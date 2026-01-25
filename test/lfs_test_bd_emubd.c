/*
 * Block device implementation using lfs_emubd for host testing
 */
#include "lfs_test_bd.h"
#include "bd/lfs_emubd.h"
#include <string.h>

/* Standard geometries for testing */
const struct lfs_test_geometry lfs_test_geometries[] = {
    {"default", 16,   16,    512,   2048},
    {"eeprom",  1,    1,     512,   2048},
    {"emmc",    512,  512,   512,   2048},
    {"nor",     1,    1,     4096,  256},
    {"nand",    4096, 4096,  32768, 32},
};

const size_t lfs_test_geometry_count =
    sizeof(lfs_test_geometries) / sizeof(lfs_test_geometries[0]);

/* Block device context for emubd */
struct lfs_test_bd {
    lfs_emubd_t emubd;
    struct lfs_emubd_config emubd_cfg;
};

int lfs_test_bd_create(struct lfs_test_bd *bd,
                       struct lfs_config *cfg,
                       const struct lfs_test_geometry *geom) {
    /* Initialize emubd config */
    memset(&bd->emubd_cfg, 0, sizeof(bd->emubd_cfg));
    bd->emubd_cfg.read_size = geom->read_size;
    bd->emubd_cfg.prog_size = geom->prog_size;
    bd->emubd_cfg.erase_size = geom->erase_size;
    bd->emubd_cfg.erase_count = geom->erase_count;
    bd->emubd_cfg.erase_value = -1;  /* Don't simulate erase value */
    bd->emubd_cfg.erase_cycles = 0;  /* No wear simulation by default */

    /* Initialize emubd state */
    memset(&bd->emubd, 0, sizeof(bd->emubd));

    /* Initialize lfs config */
    memset(cfg, 0, sizeof(*cfg));
    cfg->context = &bd->emubd;
    cfg->read = lfs_emubd_read;
    cfg->prog = lfs_emubd_prog;
    cfg->erase = lfs_emubd_erase;
    cfg->sync = lfs_emubd_sync;
    cfg->read_size = geom->read_size;
    cfg->prog_size = geom->prog_size;
    cfg->block_size = geom->erase_size;
    cfg->block_count = geom->erase_count;
    cfg->block_cycles = -1;
    cfg->cache_size = 64;
    cfg->lookahead_size = 16;

    return lfs_emubd_create(cfg, &bd->emubd_cfg);
}

int lfs_test_bd_destroy(struct lfs_test_bd *bd,
                        struct lfs_config *cfg) {
    return lfs_emubd_destroy(cfg);
}
