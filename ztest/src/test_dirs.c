/*
 * Directory operation tests - creation, removal, rename, listing
 */
#include "lfs_ztest_common.h"
#include <stdio.h>

static void *dirs_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void dirs_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(dirs_tests, NULL, dirs_setup, dirs_before, NULL, NULL);

/* Test root directory */
ZTEST(dirs_tests, test_root)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_dir_t dir;
    zassert_equal(lfs_dir_open(&lfs, &dir, "/"), 0, "Open root failed");

    struct lfs_info info;
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read . failed");
    zassert_equal(info.type, LFS_TYPE_DIR, ". not a dir");
    zassert_equal(strcmp(info.name, "."), 0, ". name wrong");

    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read .. failed");
    zassert_equal(info.type, LFS_TYPE_DIR, ".. not a dir");
    zassert_equal(strcmp(info.name, ".."), 0, ".. name wrong");

    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 0, "Should be empty");
    zassert_equal(lfs_dir_close(&lfs, &dir), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Many directory creation */
ZTEST(dirs_tests, test_many_creation)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 10;
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%03d", i);
        zassert_equal(lfs_mkdir(&lfs, path), 0, "Mkdir %s failed", path);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify directories exist */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    lfs_dir_t dir;
    zassert_equal(lfs_dir_open(&lfs, &dir, "/"), 0, "Open root failed");

    struct lfs_info info;
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read . failed");
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read .. failed");

    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "dir%03d", i);
        zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read %s failed", path);
        zassert_equal(info.type, LFS_TYPE_DIR, "%s not a dir", path);
        zassert_equal(strcmp(info.name, path), 0, "%s name wrong", path);
    }
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 0, "Extra entries");
    zassert_equal(lfs_dir_close(&lfs, &dir), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Directory removal */
ZTEST(dirs_tests, test_many_removal)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 5;
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "removeme%03d", i);
        zassert_equal(lfs_mkdir(&lfs, path), 0, "Mkdir %s failed", path);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Remove all directories */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "removeme%03d", i);
        zassert_equal(lfs_remove(&lfs, path), 0, "Remove %s failed", path);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify empty */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount2 failed");
    lfs_dir_t dir;
    zassert_equal(lfs_dir_open(&lfs, &dir, "/"), 0, "Open root failed");
    struct lfs_info info;
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read . failed");
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read .. failed");
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 0, "Should be empty");
    zassert_equal(lfs_dir_close(&lfs, &dir), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Directory rename */
ZTEST(dirs_tests, test_many_rename)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 5;
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "test%03d", i);
        zassert_equal(lfs_mkdir(&lfs, path), 0, "Mkdir %s failed", path);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Rename all directories */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    for (int i = 0; i < N; i++) {
        char oldpath[64], newpath[64];
        snprintf(oldpath, sizeof(oldpath), "test%03d", i);
        snprintf(newpath, sizeof(newpath), "tedd%03d", i);
        zassert_equal(lfs_rename(&lfs, oldpath, newpath), 0,
                      "Rename %s->%s failed", oldpath, newpath);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify renamed */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount2 failed");
    for (int i = 0; i < N; i++) {
        char path[64];
        struct lfs_info info;
        snprintf(path, sizeof(path), "tedd%03d", i);
        zassert_equal(lfs_stat(&lfs, path, &info), 0, "Stat %s failed", path);
        zassert_equal(info.type, LFS_TYPE_DIR, "%s not a dir", path);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* File creation in directories */
ZTEST(dirs_tests, test_file_creation)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    const int N = 5;
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "file%03d", i);
        lfs_file_t file;
        zassert_equal(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0,
                "Create %s failed", path);
        zassert_equal(lfs_file_close(&lfs, &file), 0, "Close %s failed", path);
    }
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify files */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    lfs_dir_t dir;
    zassert_equal(lfs_dir_open(&lfs, &dir, "/"), 0, "Open root failed");
    struct lfs_info info;
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read . failed");
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read .. failed");
    for (int i = 0; i < N; i++) {
        char path[64];
        snprintf(path, sizeof(path), "file%03d", i);
        zassert_equal(lfs_dir_read(&lfs, &dir, &info), 1, "Read %s failed", path);
        zassert_equal(info.type, LFS_TYPE_REG, "%s not a file", path);
        zassert_equal(strcmp(info.name, path), 0, "%s name wrong", path);
    }
    zassert_equal(lfs_dir_read(&lfs, &dir, &info), 0, "Extra entries");
    zassert_equal(lfs_dir_close(&lfs, &dir), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Nested directories */
ZTEST(dirs_tests, test_nested)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "a"), 0, "mkdir a failed");
    zassert_equal(lfs_mkdir(&lfs, "a/b"), 0, "mkdir a/b failed");
    zassert_equal(lfs_mkdir(&lfs, "a/b/c"), 0, "mkdir a/b/c failed");

    /* Create file in nested dir */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "a/b/c/deep",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Create failed");
    zassert_equal(lfs_file_write(&lfs, &file, "nested!", 7), 7, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");

    /* Verify */
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Remount failed");
    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "a/b/c/deep", &info), 0, "Stat failed");
    zassert_equal(info.type, LFS_TYPE_REG, "Not a file");
    zassert_equal(info.size, 7, "Size wrong");
    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Directory errors */
ZTEST(dirs_tests, test_errors)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Create a directory and file */
    zassert_equal(lfs_mkdir(&lfs, "mydir"), 0, "mkdir failed");
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "myfile",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Create failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Try to create existing dir */
    zassert_equal(lfs_mkdir(&lfs, "mydir"), LFS_ERR_EXIST, "Should fail");

    /* Try to remove non-empty parent (create file in dir first) */
    zassert_equal(lfs_file_open(&lfs, &file, "mydir/child",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Create child failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_remove(&lfs, "mydir"), LFS_ERR_NOTEMPTY, "Should fail");

    /* Try to open file as directory */
    lfs_dir_t dir;
    zassert_equal(lfs_dir_open(&lfs, &dir, "myfile"), LFS_ERR_NOTDIR,
                  "Should fail");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
