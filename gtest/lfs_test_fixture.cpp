#include "lfs_test_fixture.h"
#include <cstring>

void LfsTestFixture::SetGeometry(const LfsGeometry& geom) {
    geometry_ = geom;
}

void LfsTestFixture::SetUp() {
    // Initialize block device config
    std::memset(&bdcfg_, 0, sizeof(bdcfg_));
    bdcfg_.read_size = geometry_.read_size;
    bdcfg_.prog_size = geometry_.prog_size;
    bdcfg_.erase_size = geometry_.erase_size;
    bdcfg_.erase_count = geometry_.erase_count;
    bdcfg_.erase_value = erase_value_;
    bdcfg_.erase_cycles = erase_cycles_;
    bdcfg_.badblock_behavior = badblock_behavior_;

    // Initialize block device state
    std::memset(&bd_, 0, sizeof(bd_));

    // Initialize lfs config
    std::memset(&cfg_, 0, sizeof(cfg_));
    cfg_.context = &bd_;  // emubd expects context to point to lfs_emubd_t
    cfg_.read = lfs_emubd_read;
    cfg_.prog = lfs_emubd_prog;
    cfg_.erase = lfs_emubd_erase;
    cfg_.sync = lfs_emubd_sync;
    cfg_.read_size = geometry_.read_size;
    cfg_.prog_size = geometry_.prog_size;
    cfg_.block_size = geometry_.erase_size;
    cfg_.block_count = geometry_.erase_count;
    cfg_.block_cycles = block_cycles_;
    cfg_.cache_size = cache_size_;
    cfg_.lookahead_size = lookahead_size_;

    int err = lfs_emubd_create(&cfg_, &bdcfg_);
    ASSERT_EQ(err, 0) << "Failed to create block device";
    bd_created_ = true;
}

void LfsTestFixture::TearDown() {
    if (bd_created_) {
        lfs_emubd_destroy(&cfg_);
        bd_created_ = false;
    }
}
