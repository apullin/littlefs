#ifndef LFS_TEST_FIXTURE_H
#define LFS_TEST_FIXTURE_H

#include <gtest/gtest.h>
#include <vector>
#include <string>

extern "C" {
#include "lfs.h"
#include "bd/lfs_emubd.h"
}

// Geometry configuration for parametric tests
struct LfsGeometry {
    const char* name;
    lfs_size_t read_size;
    lfs_size_t prog_size;
    lfs_size_t erase_size;
    lfs_size_t erase_count;

    // For gtest naming
    std::string Name() const { return name; }
};

// Standard geometries for testing
inline std::vector<LfsGeometry> AllGeometries() {
    return {
        {"default", 16,   16,    512,   2048},
        {"eeprom",  1,    1,     512,   2048},
        {"emmc",    512,  512,   512,   2048},
        {"nor",     1,    1,     4096,  256},
        {"nand",    4096, 4096,  32768, 32},
    };
}

// Base fixture providing lfs_config and block device
class LfsTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    // Configure geometry before SetUp
    void SetGeometry(const LfsGeometry& geom);

    // Core test objects
    lfs_t lfs_;
    lfs_config cfg_;
    lfs_emubd_t bd_;
    lfs_emubd_config bdcfg_;

    // Configuration options (can be overridden before SetUp)
    int32_t block_cycles_ = -1;
    lfs_size_t cache_size_ = 64;
    lfs_size_t lookahead_size_ = 16;
    int32_t erase_value_ = -1;  // -1 = don't simulate erase
    uint32_t erase_cycles_ = 0;
    lfs_emubd_badblock_behavior_t badblock_behavior_ = LFS_EMUBD_BADBLOCK_PROGERROR;

private:
    LfsGeometry geometry_ = {"default", 16, 16, 512, 2048};
    bool bd_created_ = false;
};

// Parametric fixture for testing across geometries
class LfsParametricTest : public LfsTestFixture,
                          public ::testing::WithParamInterface<LfsGeometry> {
protected:
    void SetUp() override {
        SetGeometry(GetParam());
        LfsTestFixture::SetUp();
    }
};

// Custom name generator for parametric tests
struct GeometryNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<LfsGeometry>& info) const {
        return info.param.name;
    }
};

#endif // LFS_TEST_FIXTURE_H
