/*
 * Path handling tests
 */
#include "lfs_ztest_common.h"

static void *paths_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void paths_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(paths_tests, NULL, paths_setup, paths_before, NULL, NULL);

/* Simple directory path */
ZTEST(paths_tests, test_simple_dir)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "tea"), 0, "mkdir failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "tea", &info), 0, "stat failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");
    zassert_equal(strcmp(info.name, "tea"), 0, "name mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Simple file path */
ZTEST(paths_tests, test_simple_file)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "coffee",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "open failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "close failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "coffee", &info), 0, "stat failed");
    zassert_equal(info.type, LFS_TYPE_REG, "type mismatch");
    zassert_equal(strcmp(info.name, "coffee"), 0, "name mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Absolute path */
ZTEST(paths_tests, test_absolute)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "/tea"), 0, "mkdir failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "/tea", &info), 0, "stat failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Redundant slashes */
ZTEST(paths_tests, test_redundant_slashes)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "tea"), 0, "mkdir failed");
    zassert_equal(lfs_mkdir(&lfs, "tea/green"), 0, "mkdir2 failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "tea//green", &info), 0, "stat // failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");

    zassert_equal(lfs_stat(&lfs, "tea///green", &info), 0, "stat /// failed");
    zassert_equal(lfs_stat(&lfs, "//tea//green", &info), 0, "stat //x// failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Trailing slashes */
ZTEST(paths_tests, test_trailing_slashes)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "tea"), 0, "mkdir failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "tea/", &info), 0, "stat / failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");

    zassert_equal(lfs_stat(&lfs, "tea//", &info), 0, "stat // failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Dots in paths */
ZTEST(paths_tests, test_dots)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "tea"), 0, "mkdir failed");
    zassert_equal(lfs_mkdir(&lfs, "tea/green"), 0, "mkdir2 failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "tea/./green", &info), 0, "stat ./x failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");

    zassert_equal(lfs_stat(&lfs, "./tea/green", &info), 0, "stat ./ failed");
    zassert_equal(lfs_stat(&lfs, "tea/green/.", &info), 0, "stat x/. failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Dotdots in paths */
ZTEST(paths_tests, test_dotdots)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "tea"), 0, "mkdir failed");
    zassert_equal(lfs_mkdir(&lfs, "tea/green"), 0, "mkdir2 failed");
    zassert_equal(lfs_mkdir(&lfs, "coffee"), 0, "mkdir3 failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "tea/green/../green", &info), 0,
                  "stat x/../x failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");

    zassert_equal(lfs_stat(&lfs, "tea/../coffee", &info), 0,
                  "stat x/../y failed");
    zassert_equal(strcmp(info.name, "coffee"), 0, "name mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Root dotdots via subdirectory */
ZTEST(paths_tests, test_root_dotdots)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "coffee"), 0, "mkdir failed");
    zassert_equal(lfs_mkdir(&lfs, "coffee/drip"), 0, "mkdir nested failed");

    struct lfs_info info;
    /* Navigate from subdirectory back to parent */
    zassert_equal(lfs_stat(&lfs, "coffee/drip/..", &info), 0, "stat .. failed");
    zassert_equal(strcmp(info.name, "coffee"), 0, "name mismatch");
    zassert_equal(lfs_stat(&lfs, "coffee/drip/../..", &info), 0, "stat ../.. failed");
    zassert_equal(strcmp(info.name, "/"), 0, "root name mismatch");
    zassert_equal(lfs_stat(&lfs, "coffee/drip/../../coffee", &info), 0, "stat back to coffee");
    zassert_equal(strcmp(info.name, "coffee"), 0, "name mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Non-existent path */
ZTEST(paths_tests, test_noent)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "nonexistent", &info), LFS_ERR_NOENT,
                  "Should fail");

    lfs_dir_t dir;
    zassert_equal(lfs_dir_open(&lfs, &dir, "nonexistent"), LFS_ERR_NOENT,
                  "Should fail");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "nonexistent", LFS_O_RDONLY),
                  LFS_ERR_NOENT, "Should fail");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Non-existent parent */
ZTEST(paths_tests, test_noent_parent)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "nonexistent/child", &info), LFS_ERR_NOENT,
                  "Should fail");

    zassert_equal(lfs_mkdir(&lfs, "nonexistent/child"), LFS_ERR_NOENT,
                  "Should fail");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "nonexistent/child",
            LFS_O_WRONLY | LFS_O_CREAT), LFS_ERR_NOENT, "Should fail");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Empty path */
ZTEST(paths_tests, test_empty)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "", &info), LFS_ERR_INVAL, "Should fail");

    zassert_equal(lfs_mkdir(&lfs, ""), LFS_ERR_INVAL, "Should fail");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Root path */
ZTEST(paths_tests, test_root)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "/", &info), 0, "stat / failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");
    zassert_equal(strcmp(info.name, "/"), 0, "name mismatch");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Root aliases */
ZTEST(paths_tests, test_root_aliases)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    struct lfs_info info;
    /* "." refers to current directory (root) */
    zassert_equal(lfs_stat(&lfs, ".", &info), 0, "stat . failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");
    zassert_equal(strcmp(info.name, "/"), 0, "name mismatch");

    /* "./" also refers to root */
    zassert_equal(lfs_stat(&lfs, "./", &info), 0, "stat ./ failed");
    zassert_equal(info.type, LFS_TYPE_DIR, "type mismatch");

    /* Note: ".." from root is not supported and returns an error */

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
