/*
 * Compatibility tests
 *
 * Tests for compatibility between different littlefs versions.
 *
 * These tests expect to be linked against two different versions of littlefs:
 * - lfs  => the new/current version of littlefs
 * - lfsp => the previous version of littlefs
 *
 * If lfsp is not linked (LFSP not defined), these tests alias the relevant
 * lfs types/functions so the tests can be run in self-test mode.
 *
 * The incompatibility tests require access to littlefs internals via wrapper.
 */

#include <gtest/gtest.h>
#include "bd/lfs_emubd.h"
#include "lfs_test_internal.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

// Alias littlefs symbols when LFSP is not defined (self-test mode)
#ifndef LFSP
#define LFSP_DISK_VERSION LFS_DISK_VERSION
#define LFSP_DISK_VERSION_MAJOR LFS_DISK_VERSION_MAJOR
#define LFSP_DISK_VERSION_MINOR LFS_DISK_VERSION_MINOR
#define lfsp_t lfs_t
#define lfsp_config lfs_config
#define lfsp_format lfs_format
#define lfsp_mount lfs_mount
#define lfsp_unmount lfs_unmount
#define lfsp_fsinfo lfs_fsinfo
#define lfsp_fs_stat lfs_fs_stat
#define lfsp_dir_t lfs_dir_t
#define lfsp_info lfs_info
#define LFSP_TYPE_REG LFS_TYPE_REG
#define LFSP_TYPE_DIR LFS_TYPE_DIR
#define lfsp_mkdir lfs_mkdir
#define lfsp_dir_open lfs_dir_open
#define lfsp_dir_read lfs_dir_read
#define lfsp_dir_close lfs_dir_close
#define lfsp_file_t lfs_file_t
#define LFSP_O_RDONLY LFS_O_RDONLY
#define LFSP_O_WRONLY LFS_O_WRONLY
#define LFSP_O_CREAT LFS_O_CREAT
#define LFSP_O_EXCL LFS_O_EXCL
#define LFSP_SEEK_SET LFS_SEEK_SET
#define lfsp_file_open lfs_file_open
#define lfsp_file_write lfs_file_write
#define lfsp_file_read lfs_file_read
#define lfsp_file_seek lfs_file_seek
#define lfsp_file_close lfs_file_close
#endif

// Test fixture for compat tests
class CompatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize block device config
        memset(&bdcfg_, 0, sizeof(bdcfg_));
        bdcfg_.read_size = 16;
        bdcfg_.prog_size = 16;
        bdcfg_.erase_size = 512;
        bdcfg_.erase_count = 2048;
        bdcfg_.erase_value = -1;

        memset(&bd_, 0, sizeof(bd_));

        // Initialize lfs config
        memset(&cfg_, 0, sizeof(cfg_));
        cfg_.context = &bd_;
        cfg_.read = lfs_emubd_read;
        cfg_.prog = lfs_emubd_prog;
        cfg_.erase = lfs_emubd_erase;
        cfg_.sync = lfs_emubd_sync;
        cfg_.read_size = 16;
        cfg_.prog_size = 16;
        cfg_.block_size = 512;
        cfg_.block_count = 2048;
        cfg_.block_cycles = -1;
        cfg_.cache_size = 64;
        cfg_.lookahead_size = 16;

        int err = lfs_emubd_create(&cfg_, &bdcfg_);
        ASSERT_EQ(err, 0);
        bd_created_ = true;
    }

    void TearDown() override {
        if (bd_created_) {
            lfs_emubd_destroy(&cfg_);
            bd_created_ = false;
        }
    }

    lfs_config cfg_;
    lfs_emubd_t bd_;
    lfs_emubd_config bdcfg_;
    bool bd_created_ = false;
};

// Skip forward/backward tests if major version doesn't match
// (only makes sense when linking against actual previous version)
#define SKIP_IF_MAJOR_MISMATCH() \
    if (LFS_DISK_VERSION_MAJOR != LFSP_DISK_VERSION_MAJOR) { \
        GTEST_SKIP() << "Major version mismatch"; \
    }

// Forward compatibility: mount filesystem created by previous version
TEST_F(CompatTest, ForwardMount) {
    SKIP_IF_MAJOR_MISMATCH();

    // Create with previous version
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_format(&lfsp, &cfgp), 0);
    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);

    // Mount with new version
    lfs_t lfs;
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Check version
    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFSP_DISK_VERSION);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Forward compatibility: read dirs
TEST_F(CompatTest, ForwardReadDirs) {
    SKIP_IF_MAJOR_MISMATCH();
    const int COUNT = 5;

    // Create with previous version
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_format(&lfsp, &cfgp), 0);

    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);
    for (int i = 0; i < COUNT; i++) {
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_EQ(lfsp_mkdir(&lfsp, name), 0);
    }
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);

    // Mount with new version
    lfs_t lfs;
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // List directories
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_STREQ(info.name, "..");

    for (int i = 0; i < COUNT; i++) {
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_DIR);
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_STREQ(info.name, name);
    }

    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Forward compatibility: read files (parametric on size)
class CompatForwardReadFilesTest : public CompatTest,
                                    public ::testing::WithParamInterface<lfs_size_t> {};

TEST_P(CompatForwardReadFilesTest, ForwardReadFiles) {
    SKIP_IF_MAJOR_MISMATCH();
    const int COUNT = 5;
    const lfs_size_t SIZE = GetParam();
    const lfs_size_t CHUNK = 4;

    // Create with previous version
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_format(&lfsp, &cfgp), 0);

    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);
    uint32_t prng = 42;
    for (int i = 0; i < COUNT; i++) {
        lfsp_file_t file;
        char name[16];
        snprintf(name, sizeof(name), "file%03d", i);
        ASSERT_EQ(lfsp_file_open(&lfsp, &file, name,
                LFSP_O_WRONLY | LFSP_O_CREAT | LFSP_O_EXCL), 0);
        for (lfs_size_t j = 0; j < SIZE; j += CHUNK) {
            uint8_t chunk[CHUNK];
            for (lfs_size_t k = 0; k < CHUNK; k++) {
                chunk[k] = TEST_PRNG(&prng) & 0xff;
            }
            ASSERT_EQ(lfsp_file_write(&lfsp, &file, chunk, CHUNK), (lfs_ssize_t)CHUNK);
        }
        ASSERT_EQ(lfsp_file_close(&lfsp, &file), 0);
    }
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);

    // Mount with new version
    lfs_t lfs;
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // List files
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..

    for (int i = 0; i < COUNT; i++) {
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        char name[16];
        snprintf(name, sizeof(name), "file%03d", i);
        ASSERT_STREQ(info.name, name);
        ASSERT_EQ(info.size, SIZE);
    }
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Read files
    prng = 42;
    for (int i = 0; i < COUNT; i++) {
        lfs_file_t file;
        char name[16];
        snprintf(name, sizeof(name), "file%03d", i);
        ASSERT_EQ(lfs_file_open(&lfs, &file, name, LFS_O_RDONLY), 0);
        for (lfs_size_t j = 0; j < SIZE; j += CHUNK) {
            uint8_t chunk[CHUNK];
            ASSERT_EQ(lfs_file_read(&lfs, &file, chunk, CHUNK), (lfs_ssize_t)CHUNK);
            for (lfs_size_t k = 0; k < CHUNK; k++) {
                ASSERT_EQ(chunk[k], TEST_PRNG(&prng) & 0xff);
            }
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(Compat, CompatForwardReadFilesTest,
    ::testing::Values(4, 32, 512, 8192));

// Forward compatibility: write dirs
TEST_F(CompatTest, ForwardWriteDirs) {
    SKIP_IF_MAJOR_MISMATCH();
    const int COUNT = 10;

    // Create with previous version, write half
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_format(&lfsp, &cfgp), 0);

    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);
    for (int i = 0; i < COUNT/2; i++) {
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_EQ(lfsp_mkdir(&lfsp, name), 0);
    }
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);

    // Mount with new version, write other half
    lfs_t lfs;
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = COUNT/2; i < COUNT; i++) {
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, name), 0);
    }

    // List all directories
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // .
    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);  // ..

    for (int i = 0; i < COUNT; i++) {
        ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 1);
        ASSERT_EQ(info.type, LFS_TYPE_DIR);
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_STREQ(info.name, name);
    }

    ASSERT_EQ(lfs_dir_read(&lfs, &dir, &info), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Backward compatibility: mount filesystem created by new version
TEST_F(CompatTest, BackwardMount) {
    // Only test if disk versions are exactly equal
    if (LFS_DISK_VERSION != LFSP_DISK_VERSION) {
        GTEST_SKIP() << "Disk version mismatch for backward compat";
    }

    // Create with new version
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mount with previous version
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);
}

// Backward compatibility: read dirs
TEST_F(CompatTest, BackwardReadDirs) {
    if (LFS_DISK_VERSION != LFSP_DISK_VERSION) {
        GTEST_SKIP() << "Disk version mismatch for backward compat";
    }
    const int COUNT = 5;

    // Create with new version
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < COUNT; i++) {
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, name), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mount with previous version
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);

    // List directories
    lfsp_dir_t dir;
    ASSERT_EQ(lfsp_dir_open(&lfsp, &dir, "/"), 0);
    struct lfsp_info info;
    ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 1);
    ASSERT_EQ(info.type, LFSP_TYPE_DIR);
    ASSERT_STREQ(info.name, ".");
    ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 1);
    ASSERT_EQ(info.type, LFSP_TYPE_DIR);
    ASSERT_STREQ(info.name, "..");

    for (int i = 0; i < COUNT; i++) {
        ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 1);
        ASSERT_EQ(info.type, LFSP_TYPE_DIR);
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_STREQ(info.name, name);
    }

    ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 0);
    ASSERT_EQ(lfsp_dir_close(&lfsp, &dir), 0);
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);
}

// Backward compatibility: write dirs
TEST_F(CompatTest, BackwardWriteDirs) {
    if (LFS_DISK_VERSION != LFSP_DISK_VERSION) {
        GTEST_SKIP() << "Disk version mismatch for backward compat";
    }
    const int COUNT = 10;

    // Create with new version, write half
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < COUNT/2; i++) {
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_EQ(lfs_mkdir(&lfs, name), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mount with previous version, write other half
    struct lfsp_config cfgp;
    memcpy(&cfgp, &cfg_, sizeof(cfgp));
    lfsp_t lfsp;
    ASSERT_EQ(lfsp_mount(&lfsp, &cfgp), 0);
    for (int i = COUNT/2; i < COUNT; i++) {
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_EQ(lfsp_mkdir(&lfsp, name), 0);
    }

    // List all directories
    lfsp_dir_t dir;
    ASSERT_EQ(lfsp_dir_open(&lfsp, &dir, "/"), 0);
    struct lfsp_info info;
    ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 1);  // .
    ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 1);  // ..

    for (int i = 0; i < COUNT; i++) {
        ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 1);
        ASSERT_EQ(info.type, LFSP_TYPE_DIR);
        char name[16];
        snprintf(name, sizeof(name), "dir%03d", i);
        ASSERT_STREQ(info.name, name);
    }

    ASSERT_EQ(lfsp_dir_read(&lfsp, &dir, &info), 0);
    ASSERT_EQ(lfsp_dir_close(&lfsp, &dir), 0);
    ASSERT_EQ(lfsp_unmount(&lfsp), 0);
}

// Incompatibility test: fail to mount after major version bump
TEST_F(CompatTest, MajorIncompat) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Bump the major version using internal access
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Get filesystem info for superblock values
    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);

    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, (lfs_block_t[2]){0, 1}), 0);

    lfs_superblock_t superblock = {
        .version     = LFS_DISK_VERSION + 0x00010000,
        .block_size  = fsinfo.block_size,
        .block_count = fsinfo.block_count,
        .name_max    = fsinfo.name_max,
        .file_max    = fsinfo.file_max,
        .attr_max    = fsinfo.attr_max,
    };
    lfs_test_superblock_tole32(&superblock);
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir, LFS_MKATTRS(
            {LFS_MKTAG(LFS_TYPE_INLINESTRUCT, 0, sizeof(superblock)),
                &superblock})), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mount should now fail
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_INVAL);
}

// Incompatibility test: fail to mount after minor version bump
TEST_F(CompatTest, MinorIncompat) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Bump the minor version using internal access
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Get filesystem info for superblock values
    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);

    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, (lfs_block_t[2]){0, 1}), 0);

    lfs_superblock_t superblock = {
        .version     = LFS_DISK_VERSION + 0x00000001,
        .block_size  = fsinfo.block_size,
        .block_count = fsinfo.block_count,
        .name_max    = fsinfo.name_max,
        .file_max    = fsinfo.file_max,
        .attr_max    = fsinfo.attr_max,
    };
    lfs_test_superblock_tole32(&superblock);
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir, LFS_MKATTRS(
            {LFS_MKTAG(LFS_TYPE_INLINESTRUCT, 0, sizeof(superblock)),
                &superblock})), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mount should now fail
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_INVAL);
}

// Test that minor version gets correctly bumped on write
TEST_F(CompatTest, MinorBump) {
    // Only test if minor version > 0
    if (LFS_DISK_VERSION_MINOR == 0) {
        GTEST_SKIP() << "Minor version is 0, cannot test minor bump";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "test",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "testtest", 8), 8);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Write an old minor version using internal access
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Get filesystem info for superblock values
    struct lfs_fsinfo fsinfo_tmp;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo_tmp), 0);

    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, (lfs_block_t[2]){0, 1}), 0);

    lfs_superblock_t superblock = {
        .version     = LFS_DISK_VERSION - 0x00000001,
        .block_size  = fsinfo_tmp.block_size,
        .block_count = fsinfo_tmp.block_count,
        .name_max    = fsinfo_tmp.name_max,
        .file_max    = fsinfo_tmp.file_max,
        .attr_max    = fsinfo_tmp.attr_max,
    };
    lfs_test_superblock_tole32(&superblock);
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir, LFS_MKATTRS(
            {LFS_MKTAG(LFS_TYPE_INLINESTRUCT, 0, sizeof(superblock)),
                &superblock})), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mount should still work (old minor version is compatible)
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION - 1);

    // Read should work
    ASSERT_EQ(lfs_file_open(&lfs, &file, "test", LFS_O_RDONLY), 0);
    uint8_t buffer[8];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 8), 8);
    ASSERT_EQ(memcmp(buffer, "testtest", 8), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Version should be unchanged after read
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION - 1);

    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Now write - should bump minor version
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION - 1);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "test", LFS_O_WRONLY | LFS_O_TRUNC), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "teeeeest", 8), 8);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Version should be bumped after write
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION);

    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Verify after remount
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "test", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, 8), 8);
    ASSERT_EQ(memcmp(buffer, "teeeeest", 8), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}
