/*
 * Superblock tests - format, mount, expand operations
 */
#include "lfs_test_fixture.h"
#include <cstring>
#include <algorithm>

class SuperblocksTest : public LfsParametricTest {};

// Simple formatting test
TEST_P(SuperblocksTest, Format) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
}

// Mount/unmount
TEST_P(SuperblocksTest, Mount) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Make sure the magic string "littlefs" is always at offset=8
TEST_P(SuperblocksTest, Magic) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Check magic string in both superblocks
    size_t read_size = std::max((size_t)16, (size_t)cfg_.read_size);
    std::vector<uint8_t> buffer(read_size);

    ASSERT_EQ(cfg_.read(&cfg_, 0, 0, buffer.data(), read_size), 0);
    ASSERT_EQ(memcmp(&buffer[8], "littlefs", 8), 0);

    ASSERT_EQ(cfg_.read(&cfg_, 1, 0, buffer.data(), read_size), 0);
    ASSERT_EQ(memcmp(&buffer[8], "littlefs", 8), 0);
}

// Mount/unmount from interpreting a previous superblock block_count
TEST_P(SuperblocksTest, MountUnknownBlockCount) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    memset(&lfs, 0, sizeof(lfs));
    struct lfs_config tweaked_cfg = cfg_;
    tweaked_cfg.block_count = 0;
    ASSERT_EQ(lfs_mount(&lfs, &tweaked_cfg), 0);
    ASSERT_EQ(lfs.block_count, cfg_.block_count);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Invalid mount (no format)
TEST_P(SuperblocksTest, InvalidMount) {
    lfs_t lfs;
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_CORRUPT);
}

// Test we can read superblock info through lfs_fs_stat
TEST_P(SuperblocksTest, Stat) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION);
    ASSERT_EQ(fsinfo.name_max, LFS_NAME_MAX);
    ASSERT_EQ(fsinfo.file_max, LFS_FILE_MAX);
    ASSERT_EQ(fsinfo.attr_max, LFS_ATTR_MAX);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test stat with tweaked params
TEST_P(SuperblocksTest, StatTweaked) {
    const lfs_size_t TWEAKED_NAME_MAX = 63;
    const lfs_size_t TWEAKED_FILE_MAX = (1 << 16) - 1;
    const lfs_size_t TWEAKED_ATTR_MAX = 512;

    struct lfs_config tweaked_cfg = cfg_;
    tweaked_cfg.name_max = TWEAKED_NAME_MAX;
    tweaked_cfg.file_max = TWEAKED_FILE_MAX;
    tweaked_cfg.attr_max = TWEAKED_ATTR_MAX;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &tweaked_cfg), 0);

    // Mount with original config and read params
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.disk_version, LFS_DISK_VERSION);
    ASSERT_EQ(fsinfo.name_max, TWEAKED_NAME_MAX);
    ASSERT_EQ(fsinfo.file_max, TWEAKED_FILE_MAX);
    ASSERT_EQ(fsinfo.attr_max, TWEAKED_ATTR_MAX);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Expanding superblock with different block_cycles
TEST_P(SuperblocksTest, Expand) {
    // Test with block_cycles = 32
    int block_cycles = 32;
    int n = 100;

    lfs_emubd_t bd;
    lfs_emubd_config bdcfg;
    lfs_config cfg;

    auto& geom = GetParam();

    memset(&bdcfg, 0, sizeof(bdcfg));
    bdcfg.read_size = geom.read_size;
    bdcfg.prog_size = geom.prog_size;
    bdcfg.erase_size = geom.erase_size;
    bdcfg.erase_count = geom.erase_count;

    memset(&cfg, 0, sizeof(cfg));
    cfg.context = &bd;
    cfg.read = lfs_emubd_read;
    cfg.prog = lfs_emubd_prog;
    cfg.erase = lfs_emubd_erase;
    cfg.sync = lfs_emubd_sync;
    cfg.read_size = geom.read_size;
    cfg.prog_size = geom.prog_size;
    cfg.block_size = geom.erase_size;
    cfg.block_count = geom.erase_count;
    cfg.block_cycles = block_cycles;
    cfg.cache_size = std::max({(lfs_size_t)64, cfg.read_size, cfg.prog_size});
    cfg.lookahead_size = 16;

    ASSERT_EQ(lfs_emubd_create(&cfg, &bdcfg), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);

    for (int i = 0; i < n; i++) {
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, "dummy",
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, "dummy", &info), 0);
        ASSERT_STREQ(info.name, "dummy");
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        ASSERT_EQ(lfs_remove(&lfs, "dummy"), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // One last check after power-cycle
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "dummy",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "dummy", &info), 0);
    ASSERT_STREQ(info.name, "dummy");
    ASSERT_EQ(info.type, LFS_TYPE_REG);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_emubd_destroy(&cfg), 0);
}

// Expand with power cycle
TEST_P(SuperblocksTest, ExpandPowerCycle) {
    int block_cycles = 32;
    int n = 100;

    lfs_emubd_t bd;
    lfs_emubd_config bdcfg;
    lfs_config cfg;

    auto& geom = GetParam();

    memset(&bdcfg, 0, sizeof(bdcfg));
    bdcfg.read_size = geom.read_size;
    bdcfg.prog_size = geom.prog_size;
    bdcfg.erase_size = geom.erase_size;
    bdcfg.erase_count = geom.erase_count;

    memset(&cfg, 0, sizeof(cfg));
    cfg.context = &bd;
    cfg.read = lfs_emubd_read;
    cfg.prog = lfs_emubd_prog;
    cfg.erase = lfs_emubd_erase;
    cfg.sync = lfs_emubd_sync;
    cfg.read_size = geom.read_size;
    cfg.prog_size = geom.prog_size;
    cfg.block_size = geom.erase_size;
    cfg.block_count = geom.erase_count;
    cfg.block_cycles = block_cycles;
    cfg.cache_size = std::max({(lfs_size_t)64, cfg.read_size, cfg.prog_size});
    cfg.lookahead_size = 16;

    ASSERT_EQ(lfs_emubd_create(&cfg, &bdcfg), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg), 0);

    for (int i = 0; i < n; i++) {
        ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);

        // Remove lingering dummy?
        struct lfs_info info;
        int err = lfs_stat(&lfs, "dummy", &info);
        ASSERT_TRUE(err == 0 || (err == LFS_ERR_NOENT && i == 0));
        if (!err) {
            ASSERT_STREQ(info.name, "dummy");
            ASSERT_EQ(info.type, LFS_TYPE_REG);
            ASSERT_EQ(lfs_remove(&lfs, "dummy"), 0);
        }

        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, "dummy",
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        ASSERT_EQ(lfs_stat(&lfs, "dummy", &info), 0);
        ASSERT_STREQ(info.name, "dummy");
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }

    // One last check after power-cycle
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "dummy", &info), 0);
    ASSERT_STREQ(info.name, "dummy");
    ASSERT_EQ(info.type, LFS_TYPE_REG);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_emubd_destroy(&cfg), 0);
}

// Magic string after expansion
TEST_P(SuperblocksTest, MagicExpand) {
    int block_cycles = 32;
    int n = 100;

    lfs_emubd_t bd;
    lfs_emubd_config bdcfg;
    lfs_config cfg;

    auto& geom = GetParam();

    memset(&bdcfg, 0, sizeof(bdcfg));
    bdcfg.read_size = geom.read_size;
    bdcfg.prog_size = geom.prog_size;
    bdcfg.erase_size = geom.erase_size;
    bdcfg.erase_count = geom.erase_count;

    memset(&cfg, 0, sizeof(cfg));
    cfg.context = &bd;
    cfg.read = lfs_emubd_read;
    cfg.prog = lfs_emubd_prog;
    cfg.erase = lfs_emubd_erase;
    cfg.sync = lfs_emubd_sync;
    cfg.read_size = geom.read_size;
    cfg.prog_size = geom.prog_size;
    cfg.block_size = geom.erase_size;
    cfg.block_count = geom.erase_count;
    cfg.block_cycles = block_cycles;
    cfg.cache_size = std::max({(lfs_size_t)64, cfg.read_size, cfg.prog_size});
    cfg.lookahead_size = 16;

    ASSERT_EQ(lfs_emubd_create(&cfg, &bdcfg), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);

    for (int i = 0; i < n; i++) {
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, "dummy",
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, "dummy", &info), 0);
        ASSERT_STREQ(info.name, "dummy");
        ASSERT_EQ(info.type, LFS_TYPE_REG);
        ASSERT_EQ(lfs_remove(&lfs, "dummy"), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Check magic string
    size_t read_size = std::max((size_t)16, (size_t)cfg.read_size);
    std::vector<uint8_t> magic(read_size);

    ASSERT_EQ(cfg.read(&cfg, 0, 0, magic.data(), read_size), 0);
    ASSERT_EQ(memcmp(&magic[8], "littlefs", 8), 0);
    ASSERT_EQ(cfg.read(&cfg, 1, 0, magic.data(), read_size), 0);
    ASSERT_EQ(memcmp(&magic[8], "littlefs", 8), 0);

    ASSERT_EQ(lfs_emubd_destroy(&cfg), 0);
}

// Unknown block count test
TEST_P(SuperblocksTest, UnknownBlocks) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    lfs_size_t BLOCK_SIZE = cfg_.block_size;
    lfs_size_t BLOCK_COUNT = cfg_.block_count;

    // Known block_size/block_count
    struct lfs_config cfg = cfg_;
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = BLOCK_COUNT;
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Unknown block_count
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = 0;
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Do some work
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "test",
            LFS_O_CREAT | LFS_O_EXCL | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "hello!", 6), 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "test", LFS_O_RDONLY), 0);
    uint8_t buffer[256];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, sizeof(buffer)), 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(memcmp(buffer, "hello!", 6), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Fewer blocks test
TEST_P(SuperblocksTest, FewerBlocks) {
    auto& geom = GetParam();
    lfs_size_t ERASE_COUNT = geom.erase_count;
    lfs_size_t BLOCK_SIZE = geom.erase_size;

    // Test with half the blocks
    lfs_size_t BLOCK_COUNT = ERASE_COUNT / 2;
    if (BLOCK_COUNT < 2) {
        GTEST_SKIP() << "Not enough blocks for this test";
    }

    lfs_emubd_t bd;
    lfs_emubd_config bdcfg;
    lfs_config cfg;

    memset(&bdcfg, 0, sizeof(bdcfg));
    bdcfg.read_size = geom.read_size;
    bdcfg.prog_size = geom.prog_size;
    bdcfg.erase_size = BLOCK_SIZE;
    bdcfg.erase_count = ERASE_COUNT;  // Full device size for emubd

    memset(&cfg, 0, sizeof(cfg));
    cfg.context = &bd;
    cfg.read = lfs_emubd_read;
    cfg.prog = lfs_emubd_prog;
    cfg.erase = lfs_emubd_erase;
    cfg.sync = lfs_emubd_sync;
    cfg.read_size = geom.read_size;
    cfg.prog_size = geom.prog_size;
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = BLOCK_COUNT;
    cfg.block_cycles = -1;
    cfg.cache_size = std::max({(lfs_size_t)64, cfg.read_size, cfg.prog_size});
    cfg.lookahead_size = 16;

    ASSERT_EQ(lfs_emubd_create(&cfg, &bdcfg), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg), 0);

    // Known block_size/block_count
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = BLOCK_COUNT;
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Incorrect block_count should fail
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = ERASE_COUNT;
    ASSERT_EQ(lfs_mount(&lfs, &cfg), LFS_ERR_INVAL);

    // Unknown block_count should work
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = 0;
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_emubd_destroy(&cfg), 0);
}

// Grow filesystem
TEST_P(SuperblocksTest, Grow) {
    auto& geom = GetParam();
    lfs_size_t ERASE_COUNT = geom.erase_count;
    lfs_size_t BLOCK_SIZE = geom.erase_size;
    lfs_size_t BLOCK_COUNT = ERASE_COUNT / 2;
    lfs_size_t BLOCK_COUNT_2 = ERASE_COUNT;

    if (BLOCK_COUNT < 2) {
        GTEST_SKIP() << "Not enough blocks for this test";
    }

    lfs_emubd_t bd;
    lfs_emubd_config bdcfg;
    lfs_config cfg;

    memset(&bdcfg, 0, sizeof(bdcfg));
    bdcfg.read_size = geom.read_size;
    bdcfg.prog_size = geom.prog_size;
    bdcfg.erase_size = BLOCK_SIZE;
    bdcfg.erase_count = ERASE_COUNT;  // Full device for growing

    memset(&cfg, 0, sizeof(cfg));
    cfg.context = &bd;
    cfg.read = lfs_emubd_read;
    cfg.prog = lfs_emubd_prog;
    cfg.erase = lfs_emubd_erase;
    cfg.sync = lfs_emubd_sync;
    cfg.read_size = geom.read_size;
    cfg.prog_size = geom.prog_size;
    cfg.block_size = BLOCK_SIZE;
    cfg.block_count = BLOCK_COUNT;
    cfg.block_cycles = -1;
    cfg.cache_size = std::max({(lfs_size_t)64, cfg.read_size, cfg.prog_size});
    cfg.lookahead_size = 16;

    ASSERT_EQ(lfs_emubd_create(&cfg, &bdcfg), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg), 0);

    // Mount with smaller block_count
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    struct lfs_fsinfo fsinfo;
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Same size is a noop
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_grow(&lfs, BLOCK_COUNT), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Grow to new size
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_grow(&lfs, BLOCK_COUNT_2), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT_2);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    cfg.block_count = BLOCK_COUNT_2;

    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_fs_stat(&lfs, &fsinfo), 0);
    ASSERT_EQ(fsinfo.block_size, BLOCK_SIZE);
    ASSERT_EQ(fsinfo.block_count, BLOCK_COUNT_2);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Mounting with the previous size should fail
    cfg.block_count = BLOCK_COUNT;
    ASSERT_EQ(lfs_mount(&lfs, &cfg), LFS_ERR_INVAL);

    cfg.block_count = BLOCK_COUNT_2;

    // Do some work
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "test",
            LFS_O_CREAT | LFS_O_EXCL | LFS_O_WRONLY), 0);
    ASSERT_EQ(lfs_file_write(&lfs, &file, "hello!", 6), 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "test", LFS_O_RDONLY), 0);
    uint8_t buffer[256];
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, sizeof(buffer)), 6);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(memcmp(buffer, "hello!", 6), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_emubd_destroy(&cfg), 0);
}

// Metadata max test
TEST_P(SuperblocksTest, MetadataMax) {
    auto& geom = GetParam();
    int n = 100;

    lfs_emubd_t bd;
    lfs_emubd_config bdcfg;
    lfs_config cfg;

    memset(&bdcfg, 0, sizeof(bdcfg));
    bdcfg.read_size = geom.read_size;
    bdcfg.prog_size = geom.prog_size;
    bdcfg.erase_size = geom.erase_size;
    bdcfg.erase_count = geom.erase_count;

    memset(&cfg, 0, sizeof(cfg));
    cfg.context = &bd;
    cfg.read = lfs_emubd_read;
    cfg.prog = lfs_emubd_prog;
    cfg.erase = lfs_emubd_erase;
    cfg.sync = lfs_emubd_sync;
    cfg.read_size = geom.read_size;
    cfg.prog_size = geom.prog_size;
    cfg.block_size = geom.erase_size;
    cfg.block_count = geom.erase_count;
    cfg.block_cycles = -1;
    cfg.cache_size = std::max({(lfs_size_t)64, cfg.read_size, cfg.prog_size});
    cfg.lookahead_size = 16;

    // Set metadata_max to half block size
    cfg.metadata_max = std::max(cfg.block_size / 2, cfg.prog_size);

    ASSERT_EQ(lfs_emubd_create(&cfg, &bdcfg), 0);

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg), 0);

    for (int i = 0; i < n; i++) {
        lfs_file_t file;
        char name[256];
        snprintf(name, sizeof(name), "hello%03x", i);
        ASSERT_EQ(lfs_file_open(&lfs, &file, name,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, name, &info), 0);
        ASSERT_STREQ(info.name, name);
        ASSERT_EQ(info.type, LFS_TYPE_REG);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_emubd_destroy(&cfg), 0);
}

// Instantiate basic superblocks tests with all geometries
INSTANTIATE_TEST_SUITE_P(
    Geometries,
    SuperblocksTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
