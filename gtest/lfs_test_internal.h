/*
 * Internal test helper functions
 *
 * Provides access to littlefs internal functions for testing purposes.
 * This header can be used from C++ test code.
 */
#ifndef LFS_TEST_INTERNAL_H
#define LFS_TEST_INTERNAL_H

#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Internal types not exposed in public header
typedef uint32_t lfs_tag_t;
typedef int32_t lfs_stag_t;

// CTZ structure (internal)
struct lfs_ctz {
    lfs_block_t head;
    lfs_size_t size;
};

// Tag manipulation macros
#define LFS_MKTAG(type, id, size) \
    (((lfs_tag_t)(type) << 20) | ((lfs_tag_t)(id) << 10) | (lfs_tag_t)(size))

#define LFS_MKTAG_IF(cond, type, id, size) \
    ((cond) ? LFS_MKTAG(type, id, size) : LFS_MKTAG(LFS_FROM_NOOP, 0, 0))

#define LFS_MKTAG_IF_ELSE(cond, type1, id1, size1, type2, id2, size2) \
    ((cond) ? LFS_MKTAG(type1, id1, size1) : LFS_MKTAG(type2, id2, size2))

// Attribute list for commits
struct lfs_attr_internal {
    lfs_tag_t tag;
    const void *buffer;
};

#define LFS_MKATTRS(...) \
    (struct lfs_attr_internal[]){__VA_ARGS__}, \
    sizeof((struct lfs_attr_internal[]){__VA_ARGS__}) / sizeof(struct lfs_attr_internal)

// Type codes (match internal definitions in lfs.c)
#define LFS_TYPE_NAME           0x000
#define LFS_TYPE_DIRSTRUCT      0x200
#define LFS_TYPE_CTZSTRUCT      0x202
#define LFS_TYPE_INLINESTRUCT   0x201
#define LFS_TYPE_HARDTAIL       0x600
#define LFS_TYPE_SOFTTAIL       0x400
#define LFS_TYPE_GLOBALS        0x7ff
#define LFS_FROM_NOOP           0x000

// Internal functions wrapped for testing
int lfs_test_init(lfs_t *lfs, const struct lfs_config *cfg);
int lfs_test_deinit(lfs_t *lfs);

int lfs_test_dir_fetch(lfs_t *lfs, lfs_mdir_t *dir, const lfs_block_t pair[2]);
lfs_stag_t lfs_test_dir_get(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_tag_t gmask, lfs_tag_t gtag, void *buffer);
int lfs_test_dir_commit(lfs_t *lfs, lfs_mdir_t *dir,
        const struct lfs_attr_internal *attrs, int attrcount);

void lfs_test_fs_prepmove(lfs_t *lfs, uint16_t id, const lfs_block_t pair[2]);

void lfs_test_superblock_tole32(lfs_superblock_t *superblock);
void lfs_test_pair_fromle32(lfs_block_t pair[2]);

// Endian conversion
uint32_t lfs_test_tole32(uint32_t val);

// Get internal mdir from public dir handle
int lfs_test_dir_getmdir(lfs_t *lfs, lfs_dir_t *dir, lfs_mdir_t *mdir);

#ifdef __cplusplus
}
#endif

#endif
