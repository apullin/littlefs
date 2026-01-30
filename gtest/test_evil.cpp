/*
 * Evil tests - corruption recovery
 *
 * Tests for recovering from conditions which shouldn't normally
 * happen during normal operation of littlefs (invalid pointers,
 * loops, etc.)
 *
 * These tests require access to littlefs internals via the test wrapper.
 */

#include <gtest/gtest.h>
#include "bd/lfs_emubd.h"
#include "lfs_test_internal.h"
#include <cstring>
#include <cstdio>

// Test fixture for evil tests
class EvilTest : public ::testing::Test {
protected:
    static constexpr lfs_block_t root_pair[2] = {0, 1};

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

// Invalid tail pointer test
class EvilInvalidTailTest : public EvilTest,
                            public ::testing::WithParamInterface<int> {};

TEST_P(EvilInvalidTailTest, InvalidTailPointer) {
    int invalset = GetParam();

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Change tail-pointer to invalid pointers
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    lfs_block_t bad_pair[2] = {
        (lfs_block_t)((invalset & 0x1) ? 0xcccccccc : 0),
        (lfs_block_t)((invalset & 0x2) ? 0xcccccccc : 0)
    };
    struct lfs_attr_internal attrs[] = {
        {LFS_MKTAG(LFS_TYPE_HARDTAIL, 0x3ff, 8), bad_pair}
    };
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir,
            attrs, LFS_ATTR_COUNT(attrs)), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Test that mount fails gracefully
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_CORRUPT);
}

INSTANTIATE_TEST_SUITE_P(Evil, EvilInvalidTailTest,
    ::testing::Values(0x3, 0x1, 0x2));

// Invalid directory pointer test
class EvilInvalidDirTest : public EvilTest,
                           public ::testing::WithParamInterface<int> {};

TEST_P(EvilInvalidDirTest, InvalidDirPointer) {
    int invalset = GetParam();

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Make a directory
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "dir_here"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Change the dir pointer to be invalid
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    // Verify id 1 is our directory
    uint8_t buffer[1024];
    lfs_stag_t tag = lfs_test_dir_get(&lfs, &mdir,
            LFS_MKTAG(0x700, 0x3ff, 0),
            LFS_MKTAG(LFS_TYPE_NAME, 1, strlen("dir_here")), buffer);
    ASSERT_EQ(tag, LFS_MKTAG(LFS_TYPE_DIR, 1, strlen("dir_here")));
    ASSERT_EQ(memcmp(buffer, "dir_here", strlen("dir_here")), 0);

    // Change dir pointer to invalid
    lfs_block_t bad_pair[2] = {
        (lfs_block_t)((invalset & 0x1) ? 0xcccccccc : 0),
        (lfs_block_t)((invalset & 0x2) ? 0xcccccccc : 0)
    };
    struct lfs_attr_internal attrs[] = {
        {LFS_MKTAG(LFS_TYPE_DIRSTRUCT, 1, 8), bad_pair}
    };
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir,
            attrs, LFS_ATTR_COUNT(attrs)), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Test that accessing our bad dir fails gracefully
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "dir_here", &info), 0);
    ASSERT_STREQ(info.name, "dir_here");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "dir_here"), LFS_ERR_CORRUPT);
    ASSERT_EQ(lfs_stat(&lfs, "dir_here/file_here", &info), LFS_ERR_CORRUPT);
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "dir_here/dir_here"), LFS_ERR_CORRUPT);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "dir_here/file_here",
            LFS_O_RDONLY), LFS_ERR_CORRUPT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "dir_here/file_here",
            LFS_O_WRONLY | LFS_O_CREAT), LFS_ERR_CORRUPT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(Evil, EvilInvalidDirTest,
    ::testing::Values(0x3, 0x1, 0x2));

// Invalid file pointer test
class EvilInvalidFileTest : public EvilTest,
                            public ::testing::WithParamInterface<lfs_size_t> {};

TEST_P(EvilInvalidFileTest, InvalidFilePointer) {
    lfs_size_t SIZE = GetParam();

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Make a file
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "file_here",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Change the file pointer to be invalid
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    // Verify id 1 is our file
    uint8_t buffer[1024];
    lfs_stag_t tag = lfs_test_dir_get(&lfs, &mdir,
            LFS_MKTAG(0x700, 0x3ff, 0),
            LFS_MKTAG(LFS_TYPE_NAME, 1, strlen("file_here")), buffer);
    ASSERT_EQ(tag, LFS_MKTAG(LFS_TYPE_REG, 1, strlen("file_here")));
    ASSERT_EQ(memcmp(buffer, "file_here", strlen("file_here")), 0);

    // Change file pointer to invalid CTZ structure
    struct lfs_ctz bad_ctz = {0xcccccccc, lfs_test_tole32(SIZE)};
    struct lfs_attr_internal attrs[] = {
        {LFS_MKTAG(LFS_TYPE_CTZSTRUCT, 1, sizeof(struct lfs_ctz)), &bad_ctz}
    };
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir,
            attrs, LFS_ATTR_COUNT(attrs)), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Test that accessing our bad file fails
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "file_here", &info), 0);
    ASSERT_STREQ(info.name, "file_here");
    ASSERT_EQ(info.type, LFS_TYPE_REG);
    ASSERT_EQ(info.size, SIZE);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "file_here", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, SIZE), LFS_ERR_CORRUPT);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Any allocs that traverse CTZ must fail
    if (SIZE > 2 * cfg_.block_size) {
        ASSERT_EQ(lfs_mkdir(&lfs, "dir_here"), LFS_ERR_CORRUPT);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(Evil, EvilInvalidFileTest,
    ::testing::Values(10, 1000, 100000));

// Invalid gstate pointer test
class EvilInvalidGstateTest : public EvilTest,
                              public ::testing::WithParamInterface<int> {};

TEST_P(EvilInvalidGstateTest, InvalidGstatePointer) {
    int invalset = GetParam();

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Create an invalid gstate
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    lfs_block_t bad_pair[2] = {
        (lfs_block_t)((invalset & 0x1) ? 0xcccccccc : 0),
        (lfs_block_t)((invalset & 0x2) ? 0xcccccccc : 0)
    };
    lfs_test_fs_prepmove(&lfs, 1, bad_pair);
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir, NULL, 0), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Mount may not fail, but first alloc should fail when trying to fix gstate
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "should_fail"), LFS_ERR_CORRUPT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(Evil, EvilInvalidGstateTest,
    ::testing::Values(0x3, 0x1, 0x2));

// Metadata-pair loop test (self-referential tail pointer)
TEST_F(EvilTest, MdirLoop) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Change tail-pointer to point to itself
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    lfs_block_t self_pair[2] = {0, 1};
    struct lfs_attr_internal attrs[] = {
        {LFS_MKTAG(LFS_TYPE_HARDTAIL, 0x3ff, 8), self_pair}
    };
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir,
            attrs, LFS_ATTR_COUNT(attrs)), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Test that mount fails gracefully
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_CORRUPT);
}

// Metadata-pair 2-length loop test
TEST_F(EvilTest, MdirLoop2) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Create a child directory
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "child"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Find child and make it point back to root
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    lfs_block_t pair[2];
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    lfs_stag_t tag = lfs_test_dir_get(&lfs, &mdir,
            LFS_MKTAG(0x7ff, 0x3ff, 0),
            LFS_MKTAG(LFS_TYPE_DIRSTRUCT, 1, sizeof(pair)), pair);
    ASSERT_EQ(tag, LFS_MKTAG(LFS_TYPE_DIRSTRUCT, 1, sizeof(pair)));
    lfs_test_pair_fromle32(pair);

    // Change child's tail to point back to root
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, pair), 0);
    lfs_block_t back_to_root[2] = {0, 1};
    struct lfs_attr_internal attrs[] = {
        {LFS_MKTAG(LFS_TYPE_HARDTAIL, 0x3ff, 8), back_to_root}
    };
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir,
            attrs, LFS_ATTR_COUNT(attrs)), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Test that mount fails gracefully
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_CORRUPT);
}

// Metadata-pair 1-length child loop test
TEST_F(EvilTest, MdirLoopChild) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Create a child directory
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "child"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Find child and make it point to itself
    ASSERT_EQ(lfs_test_init(&lfs, &cfg_), 0);
    lfs_mdir_t mdir;
    lfs_block_t pair[2];
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, root_pair), 0);

    lfs_stag_t tag = lfs_test_dir_get(&lfs, &mdir,
            LFS_MKTAG(0x7ff, 0x3ff, 0),
            LFS_MKTAG(LFS_TYPE_DIRSTRUCT, 1, sizeof(pair)), pair);
    ASSERT_EQ(tag, LFS_MKTAG(LFS_TYPE_DIRSTRUCT, 1, sizeof(pair)));
    lfs_test_pair_fromle32(pair);

    // Change child's tail to point to itself
    ASSERT_EQ(lfs_test_dir_fetch(&lfs, &mdir, pair), 0);
    struct lfs_attr_internal attrs[] = {
        {LFS_MKTAG(LFS_TYPE_HARDTAIL, 0x3ff, 8), pair}
    };
    ASSERT_EQ(lfs_test_dir_commit(&lfs, &mdir,
            attrs, LFS_ATTR_COUNT(attrs)), 0);
    ASSERT_EQ(lfs_test_deinit(&lfs), 0);

    // Test that mount fails gracefully
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_CORRUPT);
}
