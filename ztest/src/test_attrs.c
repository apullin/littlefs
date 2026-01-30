/*
 * Extended attributes tests
 */
#include "lfs_ztest_common.h"

static void *attrs_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void attrs_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(attrs_tests, NULL, attrs_setup, attrs_before, NULL, NULL);

/* Get/set attributes */
ZTEST(attrs_tests, test_getset)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Create a file */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "attrfile",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "test", 4), 4, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Set attribute */
    const char *attrdata = "myattr";
    zassert_equal(lfs_setattr(&lfs, "attrfile", 'A', attrdata, strlen(attrdata)), 0,
                  "Setattr failed");

    /* Get attribute */
    char buffer[32];
    lfs_ssize_t len = lfs_getattr(&lfs, "attrfile", 'A', buffer, sizeof(buffer));
    zassert_equal(len, (lfs_ssize_t)strlen(attrdata), "Getattr size wrong");
    zassert_equal(memcmp(buffer, attrdata, len), 0, "Attr data mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify persists across mount */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    len = lfs_getattr(&lfs, "attrfile", 'A', buffer, sizeof(buffer));
    zassert_equal(len, (lfs_ssize_t)strlen(attrdata), "Getattr2 size wrong");
    zassert_equal(memcmp(buffer, attrdata, len), 0, "Attr2 data mismatch");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Root attributes */
ZTEST(attrs_tests, test_root_attrs)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Set attribute on root */
    const char *rootattr = "rootdata";
    zassert_equal(lfs_setattr(&lfs, "/", 'R', rootattr, strlen(rootattr)), 0,
                  "Setattr root failed");

    /* Get it back */
    char buffer[32];
    lfs_ssize_t len = lfs_getattr(&lfs, "/", 'R', buffer, sizeof(buffer));
    zassert_equal(len, (lfs_ssize_t)strlen(rootattr), "Getattr size wrong");
    zassert_equal(memcmp(buffer, rootattr, len), 0, "Root attr mismatch");

    /* Also try "." */
    len = lfs_getattr(&lfs, ".", 'R', buffer, sizeof(buffer));
    zassert_equal(len, (lfs_ssize_t)strlen(rootattr), "Getattr . size wrong");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* File config with attributes */
ZTEST(attrs_tests, test_file_config)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Open file with attribute in config */
    uint8_t attr_buffer[64];
    memset(attr_buffer, 0, sizeof(attr_buffer));
    strcpy((char *)attr_buffer, "file_attr");

    struct lfs_attr attrs[] = {
        {.type = 'T', .buffer = attr_buffer, .size = 9}
    };
    struct lfs_file_config cfg = {
        .attrs = attrs,
        .attr_count = 1,
    };

    lfs_file_t file;
    zassert_equal(lfs_file_opencfg(&lfs, &file, "cfgfile",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL, &cfg), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "hello", 5), 5, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Read attribute back */
    char buffer[32];
    lfs_ssize_t len = lfs_getattr(&lfs, "cfgfile", 'T', buffer, sizeof(buffer));
    zassert_equal(len, 9, "Attr size wrong");
    zassert_equal(memcmp(buffer, "file_attr", 9), 0, "Attr mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Remove attribute */
ZTEST(attrs_tests, test_remove_attr)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Create file and set attr */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "rmattr",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_setattr(&lfs, "rmattr", 'X', "toremove", 8), 0,
                  "Setattr failed");

    /* Verify it exists */
    char buffer[32];
    zassert_equal(lfs_getattr(&lfs, "rmattr", 'X', buffer, sizeof(buffer)), 8,
                  "Getattr failed");

    /* Remove by setting size 0 */
    zassert_equal(lfs_removeattr(&lfs, "rmattr", 'X'), 0, "Removeattr failed");

    /* Verify gone */
    zassert_equal(lfs_getattr(&lfs, "rmattr", 'X', buffer, sizeof(buffer)),
                  LFS_ERR_NOATTR, "Should be gone");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
