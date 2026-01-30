/*
 * Superblock tests - format, mount, expand operations
 */
#include "lfs_ztest_common.h"

static void *superblocks_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void superblocks_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(superblocks_tests, NULL, superblocks_setup, superblocks_before, NULL, NULL);

/* Simple formatting test */
ZTEST(superblocks_tests, test_format)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
}

/* Mount/unmount test */
ZTEST(superblocks_tests, test_mount)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Verify magic string at offset 8 */
ZTEST(superblocks_tests, test_magic)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");

    /* Check magic string in both superblocks */
    uint8_t buffer[64];
    lfs_size_t read_size = ztest_cfg.read_size > 16 ? ztest_cfg.read_size : 16;

    zassert_equal(ztest_cfg.read(&ztest_cfg, 0, 0, buffer, read_size), 0,
                  "Read block 0 failed");
    zassert_equal(memcmp(&buffer[8], "littlefs", 8), 0,
                  "Magic mismatch in block 0");

    zassert_equal(ztest_cfg.read(&ztest_cfg, 1, 0, buffer, read_size), 0,
                  "Read block 1 failed");
    zassert_equal(memcmp(&buffer[8], "littlefs", 8), 0,
                  "Magic mismatch in block 1");
}

/* Mount with unknown block count */
ZTEST(superblocks_tests, test_mount_unknown_block_count)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");

    memset(&lfs, 0, sizeof(lfs));
    struct lfs_config tweaked_cfg = ztest_cfg;
    tweaked_cfg.block_count = 0;
    zassert_equal(lfs_mount(&lfs, &tweaked_cfg), 0, "Mount with unknown count failed");
    zassert_equal(lfs.block_count, ztest_cfg.block_count, "Block count mismatch");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Invalid mount (no format) */
ZTEST(superblocks_tests, test_invalid_mount)
{
    lfs_t lfs;
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), LFS_ERR_CORRUPT,
                  "Mount should fail on unformatted device");
}

/* Stat filesystem info */
ZTEST(superblocks_tests, test_stat)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    struct lfs_fsinfo fsinfo;
    zassert_equal(lfs_fs_stat(&lfs, &fsinfo), 0, "Stat failed");
    zassert_equal(fsinfo.disk_version, LFS_DISK_VERSION, "Version mismatch");
    zassert_equal(fsinfo.block_size, ztest_cfg.block_size, "Block size mismatch");
    zassert_equal(fsinfo.block_count, ztest_cfg.block_count, "Block count mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
