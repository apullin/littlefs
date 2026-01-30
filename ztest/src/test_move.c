/*
 * Move/rename tests
 */
#include "lfs_ztest_common.h"

static void *move_setup(void)
{
    lfs_ztest_format_device();
    return NULL;
}

static void move_before(void *f)
{
    ARG_UNUSED(f);
    lfs_ztest_format_device();
}

ZTEST_SUITE(move_tests, NULL, move_setup, move_before, NULL, NULL);

/* Move file */
ZTEST(move_tests, test_move_file)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "src",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "test", 4), 4, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Move */
    zassert_equal(lfs_rename(&lfs, "src", "dst"), 0, "Rename failed");

    /* Verify old is gone, new exists */
    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "src", &info), LFS_ERR_NOENT, "Old exists");
    zassert_equal(lfs_stat(&lfs, "dst", &info), 0, "New missing");
    zassert_equal(info.type, LFS_TYPE_REG, "Type mismatch");

    /* Verify data */
    uint8_t buffer[8];
    zassert_equal(lfs_file_open(&lfs, &file, "dst", LFS_O_RDONLY), 0,
                  "Open new failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 8), 4, "Read failed");
    zassert_equal(memcmp(buffer, "test", 4), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Move directory */
ZTEST(move_tests, test_move_dir)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    zassert_equal(lfs_mkdir(&lfs, "srcdir"), 0, "Mkdir failed");

    /* Create file inside */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "srcdir/child",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_write(&lfs, &file, "nested", 6), 6, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Move directory */
    zassert_equal(lfs_rename(&lfs, "srcdir", "dstdir"), 0, "Rename failed");

    /* Verify */
    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "srcdir", &info), LFS_ERR_NOENT, "Old exists");
    zassert_equal(lfs_stat(&lfs, "dstdir", &info), 0, "New missing");
    zassert_equal(info.type, LFS_TYPE_DIR, "Type mismatch");

    /* Verify nested file moved too */
    zassert_equal(lfs_stat(&lfs, "dstdir/child", &info), 0, "Child missing");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Move with overwrite */
ZTEST(move_tests, test_move_overwrite)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    lfs_file_t file;
    /* Create source file */
    zassert_equal(lfs_file_open(&lfs, &file, "src",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open src failed");
    zassert_equal(lfs_file_write(&lfs, &file, "source", 6), 6, "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Create destination file */
    zassert_equal(lfs_file_open(&lfs, &file, "dst",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open dst failed");
    zassert_equal(lfs_file_write(&lfs, &file, "destination", 11), 11,
                  "Write failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    /* Move (overwrite) */
    zassert_equal(lfs_rename(&lfs, "src", "dst"), 0, "Rename failed");

    /* Verify src is gone, dst has src's content */
    struct lfs_info info;
    zassert_equal(lfs_stat(&lfs, "src", &info), LFS_ERR_NOENT, "Old exists");
    zassert_equal(lfs_stat(&lfs, "dst", &info), 0, "New missing");
    zassert_equal(info.size, 6, "Size mismatch");

    uint8_t buffer[16];
    zassert_equal(lfs_file_open(&lfs, &file, "dst", LFS_O_RDONLY), 0,
                  "Open failed");
    zassert_equal(lfs_file_read(&lfs, &file, buffer, 16), 6, "Read failed");
    zassert_equal(memcmp(buffer, "source", 6), 0, "Data mismatch");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}

/* Move errors */
ZTEST(move_tests, test_move_errors)
{
    lfs_t lfs;
    zassert_equal(lfs_format(&lfs, &ztest_cfg), 0, "Format failed");
    zassert_equal(lfs_mount(&lfs, &ztest_cfg), 0, "Mount failed");

    /* Non-existent source */
    zassert_equal(lfs_rename(&lfs, "nonexistent", "dst"), LFS_ERR_NOENT,
                  "Should fail");

    /* Create a file and directory */
    lfs_file_t file;
    zassert_equal(lfs_file_open(&lfs, &file, "myfile",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");
    zassert_equal(lfs_mkdir(&lfs, "mydir"), 0, "Mkdir failed");

    /* Non-existent parent of destination */
    zassert_equal(lfs_rename(&lfs, "myfile", "nonexistent/dst"),
                  LFS_ERR_NOENT, "Should fail");

    /* Can't overwrite non-empty directory */
    zassert_equal(lfs_file_open(&lfs, &file, "mydir/child",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0, "Open child failed");
    zassert_equal(lfs_file_close(&lfs, &file), 0, "Close failed");

    zassert_equal(lfs_mkdir(&lfs, "otherdir"), 0, "Mkdir2 failed");
    zassert_equal(lfs_rename(&lfs, "otherdir", "mydir"), LFS_ERR_NOTEMPTY,
                  "Should fail");

    zassert_equal(lfs_unmount(&lfs), 0, "Unmount failed");
}
