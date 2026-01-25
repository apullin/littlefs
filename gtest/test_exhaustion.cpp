/*
 * Exhaustion tests
 *
 * Tests running a filesystem to exhaustion (all blocks worn out).
 * Tests wear leveling by verifying that increased space translates
 * to increased lifetime.
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

// Exhaustion test parameters
struct ExhaustionParams {
    const char* name;
    lfs_emubd_badblock_behavior_t behavior;
};

inline std::vector<ExhaustionParams> AllExhaustionParams() {
    return {
        {"progerror", LFS_EMUBD_BADBLOCK_PROGERROR},
        {"eraseerror", LFS_EMUBD_BADBLOCK_ERASEERROR},
        {"readerror", LFS_EMUBD_BADBLOCK_READERROR},
        {"prognoop", LFS_EMUBD_BADBLOCK_PROGNOOP},
        {"erasenoop", LFS_EMUBD_BADBLOCK_ERASENOOP},
    };
}

// Custom fixture for exhaustion tests
class ExhaustionTest : public LfsTestFixture,
                       public ::testing::WithParamInterface<ExhaustionParams> {
protected:
    void SetUp() override {
        // Use smaller block count for faster tests
        LfsGeometry geom = {"default", 16, 16, 512, 256};
        SetGeometry(geom);

        // Configure for exhaustion testing
        erase_cycles_ = 10;
        block_cycles_ = erase_cycles_ / 2;
        badblock_behavior_ = GetParam().behavior;

        LfsTestFixture::SetUp();
    }
};

struct ExhaustionParamNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<ExhaustionParams>& info) const {
        return info.param.name;
    }
};

// Run filesystem to exhaustion, return cycle count
static uint32_t run_to_exhaustion(lfs_t* lfs, lfs_config* cfg,
                                   const char* dir, int files) {
    uint32_t cycle = 0;
    bool exhausted = false;

    while (!exhausted) {
        EXPECT_EQ(lfs_mount(lfs, cfg), 0);

        for (int i = 0; i < files && !exhausted; i++) {
            char path[256];
            if (dir) {
                snprintf(path, sizeof(path), "%s/test%d", dir, i);
            } else {
                snprintf(path, sizeof(path), "test%d", i);
            }

            uint32_t prng = cycle * i;
            lfs_size_t size = 1 << ((TEST_PRNG(&prng) % 10) + 2);

            lfs_file_t file;
            EXPECT_EQ(lfs_file_open(lfs, &file, path,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);

            for (lfs_size_t j = 0; j < size && !exhausted; j++) {
                char c = 'a' + (TEST_PRNG(&prng) % 26);
                lfs_ssize_t res = lfs_file_write(lfs, &file, &c, 1);
                if (res == LFS_ERR_NOSPC) {
                    lfs_file_close(lfs, &file);
                    lfs_unmount(lfs);
                    exhausted = true;
                    break;
                }
                EXPECT_EQ(res, 1);
            }

            if (!exhausted) {
                int err = lfs_file_close(lfs, &file);
                if (err == LFS_ERR_NOSPC) {
                    lfs_unmount(lfs);
                    exhausted = true;
                    break;
                }
                EXPECT_EQ(err, 0);
            }
        }

        if (!exhausted) {
            // Verify files
            for (int i = 0; i < files; i++) {
                char path[256];
                if (dir) {
                    snprintf(path, sizeof(path), "%s/test%d", dir, i);
                } else {
                    snprintf(path, sizeof(path), "test%d", i);
                }

                uint32_t prng = cycle * i;
                lfs_size_t size = 1 << ((TEST_PRNG(&prng) % 10) + 2);

                lfs_file_t file;
                EXPECT_EQ(lfs_file_open(lfs, &file, path, LFS_O_RDONLY), 0);

                for (lfs_size_t j = 0; j < size; j++) {
                    char expected = 'a' + (TEST_PRNG(&prng) % 26);
                    char actual;
                    EXPECT_EQ(lfs_file_read(lfs, &file, &actual, 1), 1);
                    EXPECT_EQ(actual, expected);
                }
                EXPECT_EQ(lfs_file_close(lfs, &file), 0);
            }
            EXPECT_EQ(lfs_unmount(lfs), 0);
            cycle++;
        }
    }

    return cycle;
}

// Test running filesystem to exhaustion (with subdirectory)
TEST_P(ExhaustionTest, Normal) {
    const int FILES = 10;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "roadrunner"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    uint32_t cycle = run_to_exhaustion(&lfs, &cfg_, "roadrunner", FILES);

    // Should still be readable after exhaustion
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < FILES; i++) {
        char path[256];
        snprintf(path, sizeof(path), "roadrunner/test%d", i);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Should have completed at least some cycles
    ASSERT_GT(cycle, 0u);
}

// Test running filesystem to exhaustion (files in root, expanding superblock)
TEST_P(ExhaustionTest, Superblocks) {
    const int FILES = 10;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    uint32_t cycle = run_to_exhaustion(&lfs, &cfg_, nullptr, FILES);

    // Should still be readable after exhaustion
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < FILES; i++) {
        char path[256];
        snprintf(path, sizeof(path), "test%d", i);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_GT(cycle, 0u);
}

// Wear-leveling test: verify that doubling space roughly doubles lifetime
TEST_P(ExhaustionTest, WearLeveling) {
    const int FILES = 10;
    const uint32_t ERASE_CYCLES = 20;

    // Override for this specific test
    erase_cycles_ = ERASE_CYCLES;
    block_cycles_ = ERASE_CYCLES / 2;

    uint32_t run_cycles[2];
    const uint32_t run_block_count[2] = {cfg_.block_count / 2, cfg_.block_count};

    for (int run = 0; run < 2; run++) {
        // Reset wear on all blocks
        // For run 0: only use half the blocks (mark other half as exhausted)
        // For run 1: use all blocks
        for (lfs_block_t b = 0; b < cfg_.block_count; b++) {
            uint32_t wear = (b < run_block_count[run]) ? 0 : ERASE_CYCLES;
            ASSERT_EQ(lfs_emubd_setwear(&cfg_, b, wear), 0);
        }

        lfs_t lfs;
        ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        ASSERT_EQ(lfs_mkdir(&lfs, "roadrunner"), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        run_cycles[run] = run_to_exhaustion(&lfs, &cfg_, "roadrunner", FILES);

        // Should still be readable
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        for (int i = 0; i < FILES; i++) {
            char path[256];
            snprintf(path, sizeof(path), "roadrunner/test%d", i);
            struct lfs_info info;
            ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
        }
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }

    // Check we increased the lifetime by roughly 2x with ~10% error tolerance
    // run_cycles[1] * 110/100 > 2 * run_cycles[0]
    EXPECT_GT(run_cycles[1] * 110 / 100, 2 * run_cycles[0])
        << "Expected wear leveling to roughly double lifetime when doubling space. "
        << "Half blocks: " << run_cycles[0] << " cycles, "
        << "Full blocks: " << run_cycles[1] << " cycles";
}

// Wear-leveling with superblock expansion
TEST_P(ExhaustionTest, WearLevelingSuperblocks) {
    const int FILES = 10;
    const uint32_t ERASE_CYCLES = 20;

    erase_cycles_ = ERASE_CYCLES;
    block_cycles_ = ERASE_CYCLES / 2;

    uint32_t run_cycles[2];
    const uint32_t run_block_count[2] = {cfg_.block_count / 2, cfg_.block_count};

    for (int run = 0; run < 2; run++) {
        for (lfs_block_t b = 0; b < cfg_.block_count; b++) {
            uint32_t wear = (b < run_block_count[run]) ? 0 : ERASE_CYCLES;
            ASSERT_EQ(lfs_emubd_setwear(&cfg_, b, wear), 0);
        }

        lfs_t lfs;
        ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

        run_cycles[run] = run_to_exhaustion(&lfs, &cfg_, nullptr, FILES);

        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        for (int i = 0; i < FILES; i++) {
            char path[256];
            snprintf(path, sizeof(path), "test%d", i);
            struct lfs_info info;
            ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
        }
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }

    EXPECT_GT(run_cycles[1] * 110 / 100, 2 * run_cycles[0])
        << "Expected wear leveling to roughly double lifetime. "
        << "Half: " << run_cycles[0] << ", Full: " << run_cycles[1];
}

// Test wear distribution - blocks should be worn roughly evenly
TEST_P(ExhaustionTest, WearDistribution) {
    const int FILES = 10;
    const int CYCLES = 100;
    const int BLOCK_CYCLES_VAL = 5;

    // Override settings
    erase_cycles_ = 0xffffffff;  // No limit on erase cycles
    block_cycles_ = BLOCK_CYCLES_VAL;

    // Need BLOCK_CYCLES < CYCLES/10
    if (BLOCK_CYCLES_VAL >= CYCLES / 10) {
        GTEST_SKIP() << "Block cycles too high for this test";
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "roadrunner"), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    uint32_t cycle = 0;
    bool exhausted = false;

    while (cycle < CYCLES && !exhausted) {
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

        for (int i = 0; i < FILES && !exhausted; i++) {
            char path[256];
            snprintf(path, sizeof(path), "roadrunner/test%d", i);

            uint32_t prng = cycle * i;
            lfs_size_t size = 1 << 4;  // Fixed size for this test

            lfs_file_t file;
            ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);

            for (lfs_size_t j = 0; j < size && !exhausted; j++) {
                char c = 'a' + (TEST_PRNG(&prng) % 26);
                lfs_ssize_t res = lfs_file_write(&lfs, &file, &c, 1);
                if (res == LFS_ERR_NOSPC) {
                    lfs_file_close(&lfs, &file);
                    lfs_unmount(&lfs);
                    exhausted = true;
                    break;
                }
                ASSERT_EQ(res, 1);
            }

            if (!exhausted) {
                int err = lfs_file_close(&lfs, &file);
                if (err == LFS_ERR_NOSPC) {
                    lfs_unmount(&lfs);
                    exhausted = true;
                    break;
                }
                ASSERT_EQ(err, 0);
            }
        }

        if (!exhausted) {
            // Verify
            for (int i = 0; i < FILES; i++) {
                char path[256];
                snprintf(path, sizeof(path), "roadrunner/test%d", i);

                uint32_t prng = cycle * i;
                lfs_size_t size = 1 << 4;

                lfs_file_t file;
                ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);

                for (lfs_size_t j = 0; j < size; j++) {
                    char expected = 'a' + (TEST_PRNG(&prng) % 26);
                    char actual;
                    ASSERT_EQ(lfs_file_read(&lfs, &file, &actual, 1), 1);
                    ASSERT_EQ(actual, expected);
                }
                ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
            }
            ASSERT_EQ(lfs_unmount(&lfs), 0);
            cycle++;
        }
    }

    // Verify still readable
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < FILES; i++) {
        char path[256];
        snprintf(path, sizeof(path), "roadrunner/test%d", i);
        struct lfs_info info;
        ASSERT_EQ(lfs_stat(&lfs, path, &info), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Check wear distribution
    lfs_emubd_wear_t minwear = 0x7fffffff;
    lfs_emubd_wear_t maxwear = 0;
    lfs_emubd_wear_t totalwear = 0;

    // Skip blocks 0 and 1 (superblock movement is intentionally avoided)
    for (lfs_block_t b = 2; b < cfg_.block_count; b++) {
        lfs_emubd_wear_t wear = lfs_emubd_wear(&cfg_, b);
        ASSERT_GE(wear, 0);
        if (wear < minwear) minwear = wear;
        if (wear > maxwear) maxwear = wear;
        totalwear += wear;
    }

    lfs_emubd_wear_t avgwear = totalwear / cfg_.block_count;

    // Calculate variance
    lfs_emubd_wear_t dev2 = 0;
    for (lfs_block_t b = 2; b < cfg_.block_count; b++) {
        lfs_emubd_wear_t wear = lfs_emubd_wear(&cfg_, b);
        lfs_emubd_swear_t diff = wear - avgwear;
        dev2 += diff * diff;
    }
    dev2 /= totalwear;

    // Standard deviation^2 should be small (< 8)
    EXPECT_LT(dev2, 8)
        << "Wear not evenly distributed. "
        << "Min: " << minwear << ", Max: " << maxwear
        << ", Avg: " << avgwear << ", Dev^2: " << dev2;
}

INSTANTIATE_TEST_SUITE_P(
    Exhaustion, ExhaustionTest,
    ::testing::ValuesIn(AllExhaustionParams()),
    ExhaustionParamNameGenerator{});
