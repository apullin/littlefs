/*
 * Power-loss tests
 *
 * Tests for specific power-loss corner cases. These tests explicitly
 * corrupt/modify block data to simulate power-loss scenarios.
 *
 * These tests require access to littlefs internals via the test wrapper.
 */

#include <gtest/gtest.h>
#include "bd/lfs_emubd.h"
#include "lfs_test_internal.h"
#include <cstring>
#include <cstdio>
#include <vector>

// Test fixture for powerloss tests
class PowerlossTest : public ::testing::Test {
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

// Test power-loss with only a revision count written
// Simulates a scenario where power is lost after writing just the revision count
TEST_F(PowerlossTest, OnlyRevisionCount) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "notebook"), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    char buffer[256];
    strcpy(buffer, "hello");
    lfs_size_t size = strlen("hello");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_sync(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Verify written data
    char rbuffer[256];
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper", LFS_O_RDONLY), 0);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Get pair/rev count using internal access
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "notebook"), 0);
    lfs_mdir_t mdir;
    lfs_test_dir_getmdir(&lfs, &dir, &mdir);
    lfs_block_t pair[2] = {mdir.pair[0], mdir.pair[1]};
    uint32_t rev = mdir.rev;
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Write just the revision count - simulating partial write during power-loss
    std::vector<uint8_t> bbuffer(cfg_.block_size);
    ASSERT_EQ(cfg_.read(&cfg_, pair[1], 0, bbuffer.data(), cfg_.block_size), 0);

    // Increment revision and write it
    uint32_t new_rev = lfs_test_tole32(rev + 1);
    memcpy(bbuffer.data(), &new_rev, sizeof(uint32_t));

    ASSERT_EQ(cfg_.erase(&cfg_, pair[1]), 0);
    ASSERT_EQ(cfg_.prog(&cfg_, pair[1], 0, bbuffer.data(), cfg_.block_size), 0);

    // Mount should still work - the filesystem should recover from partial write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Can we still read?
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper", LFS_O_RDONLY), 0);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Can we write?
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper",
            LFS_O_WRONLY | LFS_O_APPEND), 0);
    strcpy(buffer, "goodbye");
    size = strlen("goodbye");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_sync(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Verify all data
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper", LFS_O_RDONLY), 0);
    strcpy(buffer, "hello");
    size = strlen("hello");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    strcpy(buffer, "goodbye");
    size = strlen("goodbye");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test power-loss with partial prog - byte corruption
// Simulates a scenario where power is lost during a prog operation
struct PartialProgParams {
    const char* name;
    int32_t byte_off;  // Use signed to allow sentinel values (-1, -2)
    uint8_t byte_value;
};

class PowerlossPartialProgTest : public PowerlossTest,
                                  public ::testing::WithParamInterface<PartialProgParams> {};

TEST_P(PowerlossPartialProgTest, PartialProg) {
    const auto& p = GetParam();

    // Skip if prog_size == block_size (no partial prog possible)
    if (cfg_.prog_size >= cfg_.block_size) {
        GTEST_SKIP() << "prog_size must be < block_size for partial prog test";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "notebook"), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

    char buffer[256];
    strcpy(buffer, "hello");
    lfs_size_t size = strlen("hello");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_sync(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Verify written data
    char rbuffer[256];
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper", LFS_O_RDONLY), 0);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Get offset to next prog using internal access
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "notebook"), 0);
    lfs_mdir_t mdir;
    lfs_test_dir_getmdir(&lfs, &dir, &mdir);
    lfs_block_t block = mdir.pair[0];
    lfs_off_t off = mdir.off;
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Tweak a byte to simulate partial prog corruption
    std::vector<uint8_t> bbuffer(cfg_.block_size);
    ASSERT_EQ(cfg_.read(&cfg_, block, 0, bbuffer.data(), cfg_.block_size), 0);

    // Calculate actual byte offset (handle parameter expressions)
    lfs_off_t actual_byte_off = p.byte_off;
    if (p.byte_off == -1) {
        // PROG_SIZE - 1
        actual_byte_off = cfg_.prog_size - 1;
    } else if (p.byte_off == -2) {
        // PROG_SIZE / 2
        actual_byte_off = cfg_.prog_size / 2;
    }

    bbuffer[off + actual_byte_off] = p.byte_value;

    ASSERT_EQ(cfg_.erase(&cfg_, block), 0);
    ASSERT_EQ(cfg_.prog(&cfg_, block, 0, bbuffer.data(), cfg_.block_size), 0);

    // Mount should still work - the filesystem should detect and recover from corruption
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Can we still read?
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper", LFS_O_RDONLY), 0);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Can we write?
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper",
            LFS_O_WRONLY | LFS_O_APPEND), 0);
    strcpy(buffer, "goodbye");
    size = strlen("goodbye");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_sync(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Verify all data
    ASSERT_EQ(lfs_file_open(&lfs, &file, "notebook/paper", LFS_O_RDONLY), 0);
    strcpy(buffer, "hello");
    size = strlen("hello");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    strcpy(buffer, "goodbye");
    size = strlen("goodbye");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(memcmp(rbuffer, buffer, size), 0);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Parameter combinations: byte_off x byte_value
// byte_off: 0, PROG_SIZE-1 (-1), PROG_SIZE/2 (-2)
// byte_value: 0x33, 0xcc
INSTANTIATE_TEST_SUITE_P(
    Powerloss, PowerlossPartialProgTest,
    ::testing::Values(
        PartialProgParams{"off0_val0x33", 0, 0x33},
        PartialProgParams{"off0_val0xcc", 0, 0xcc},
        PartialProgParams{"offEnd_val0x33", -1, 0x33},  // PROG_SIZE - 1
        PartialProgParams{"offEnd_val0xcc", -1, 0xcc},
        PartialProgParams{"offMid_val0x33", -2, 0x33},  // PROG_SIZE / 2
        PartialProgParams{"offMid_val0xcc", -2, 0xcc}
    ),
    [](const ::testing::TestParamInfo<PartialProgParams>& info) {
        return info.param.name;
    });
