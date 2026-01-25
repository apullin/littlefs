/*
 * Internal test helper functions implementation
 *
 * This file includes lfs.c directly to get access to internal functions,
 * then provides wrapper functions that can be called from C++ test code.
 */

// Include the full implementation
#include "lfs.c"

// Define the external attr struct (matches header definition)
struct lfs_attr_internal {
    lfs_tag_t tag;
    const void *buffer;
};

// Wrapper implementations
int lfs_test_init(lfs_t *lfs, const struct lfs_config *cfg) {
    return lfs_init(lfs, cfg);
}

int lfs_test_deinit(lfs_t *lfs) {
    return lfs_deinit(lfs);
}

int lfs_test_dir_fetch(lfs_t *lfs, lfs_mdir_t *dir, const lfs_block_t pair[2]) {
    return lfs_dir_fetch(lfs, dir, pair);
}

lfs_stag_t lfs_test_dir_get(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_tag_t gmask, lfs_tag_t gtag, void *buffer) {
    return lfs_dir_get(lfs, dir, gmask, gtag, buffer);
}

int lfs_test_dir_commit(lfs_t *lfs, lfs_mdir_t *dir,
        const struct lfs_attr_internal *attrs, int attrcount) {
    // Convert from our external attr format to internal
    // The internal format uses lfs_mattr
    struct lfs_mattr mattrs[16];
    if (attrcount > 16) {
        return LFS_ERR_INVAL;
    }

    for (int i = 0; i < attrcount; i++) {
        mattrs[i].tag = attrs[i].tag;
        mattrs[i].buffer = attrs[i].buffer;
    }

    return lfs_dir_commit(lfs, dir, mattrs, attrcount);
}

void lfs_test_fs_prepmove(lfs_t *lfs, uint16_t id, const lfs_block_t pair[2]) {
    lfs_fs_prepmove(lfs, id, pair);
}

void lfs_test_superblock_tole32(lfs_superblock_t *superblock) {
    lfs_superblock_tole32(superblock);
}

void lfs_test_pair_fromle32(lfs_block_t pair[2]) {
    lfs_pair_fromle32(pair);
}

uint32_t lfs_test_tole32(uint32_t val) {
    return lfs_tole32(val);
}

// Get internal mdir from public dir handle
int lfs_test_dir_getmdir(lfs_t *lfs, lfs_dir_t *dir, lfs_mdir_t *mdir) {
    (void)lfs;
    // Copy from the internal dir.m to our mdir struct
    mdir->pair[0] = dir->m.pair[0];
    mdir->pair[1] = dir->m.pair[1];
    mdir->rev = dir->m.rev;
    mdir->off = dir->m.off;
    mdir->etag = dir->m.etag;
    mdir->count = dir->m.count;
    mdir->erased = dir->m.erased;
    mdir->split = dir->m.split;
    mdir->tail[0] = dir->m.tail[0];
    mdir->tail[1] = dir->m.tail[1];
    return 0;
}
