/*
 * Bad block tests
 *
 * Tests filesystem behavior when blocks go bad (wear out).
 * Uses emubd's setwear() to mark blocks as bad and tests various
 * bad block behaviors (errors vs noops).
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

// Bad block test parameters
struct BadBlockParams {
    const char* name;
    lfs_emubd_badblock_behavior_t behavior;
    int32_t erase_value;
};

// All combinations of bad block behavior and erase values
inline std::vector<BadBlockParams> AllBadBlockParams() {
    std::vector<BadBlockParams> params;
    const lfs_emubd_badblock_behavior_t behaviors[] = {
        LFS_EMUBD_BADBLOCK_PROGERROR,
        LFS_EMUBD_BADBLOCK_ERASEERROR,
        LFS_EMUBD_BADBLOCK_READERROR,
        LFS_EMUBD_BADBLOCK_PROGNOOP,
        LFS_EMUBD_BADBLOCK_ERASENOOP,
    };
    const char* behavior_names[] = {
        "progerror", "eraseerror", "readerror", "prognoop", "erasenoop"
    };
    const int32_t erase_values[] = {0x00, 0xff, -1};
    const char* erase_names[] = {"erase0", "eraseff", "erasenone"};

    for (int b = 0; b < 5; b++) {
        for (int e = 0; e < 3; e++) {
            char name[64];
            snprintf(name, sizeof(name), "%s_%s", behavior_names[b], erase_names[e]);
            params.push_back({strdup(name), behaviors[b], erase_values[e]});
        }
    }
    return params;
}

// Custom fixture for bad block tests
class BadBlockTest : public LfsTestFixture,
                     public ::testing::WithParamInterface<BadBlockParams> {
protected:
    void SetUp() override {
        // Use smaller block count for faster tests
        LfsGeometry geom = {"default", 16, 16, 512, 256};
        SetGeometry(geom);

        // Configure bad block behavior
        erase_cycles_ = 0xffffffff;  // Enable wear tracking
        badblock_behavior_ = GetParam().behavior;
        erase_value_ = GetParam().erase_value;

        LfsTestFixture::SetUp();
    }
};

struct BadBlockParamNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<BadBlockParams>& info) const {
        return info.param.name;
    }
};

// Helper to create dirs and files
static void create_content(lfs_t* lfs, int namemult, int filemult) {
    for (int i = 1; i < 10; i++) {
        uint8_t buffer[1024];
        for (int j = 0; j < namemult; j++) {
            buffer[j] = '0' + i;
        }
        buffer[namemult] = '\0';
        ASSERT_EQ(lfs_mkdir(lfs, (char*)buffer), 0);

        buffer[namemult] = '/';
        for (int j = 0; j < namemult; j++) {
            buffer[j + namemult + 1] = '0' + i;
        }
        buffer[2 * namemult + 1] = '\0';

        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(lfs, &file, (char*)buffer,
                LFS_O_WRONLY | LFS_O_CREAT), 0);

        lfs_size_t size = namemult;
        for (int j = 0; j < i * filemult; j++) {
            ASSERT_EQ(lfs_file_write(lfs, &file, buffer, size), (lfs_ssize_t)size);
        }
        ASSERT_EQ(lfs_file_close(lfs, &file), 0);
    }
}

// Helper to verify dirs and files
static void verify_content(lfs_t* lfs, int namemult, int filemult) {
    for (int i = 1; i < 10; i++) {
        uint8_t buffer[1024];
        for (int j = 0; j < namemult; j++) {
            buffer[j] = '0' + i;
        }
        buffer[namemult] = '\0';

        struct lfs_info info;
        ASSERT_EQ(lfs_stat(lfs, (char*)buffer, &info), 0);
        ASSERT_EQ(info.type, LFS_TYPE_DIR);

        buffer[namemult] = '/';
        for (int j = 0; j < namemult; j++) {
            buffer[j + namemult + 1] = '0' + i;
        }
        buffer[2 * namemult + 1] = '\0';

        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(lfs, &file, (char*)buffer, LFS_O_RDONLY), 0);

        lfs_size_t size = namemult;
        for (int j = 0; j < i * filemult; j++) {
            uint8_t rbuffer[1024];
            ASSERT_EQ(lfs_file_read(lfs, &file, rbuffer, size), (lfs_ssize_t)size);
            ASSERT_EQ(memcmp(buffer, rbuffer, size), 0);
        }
        ASSERT_EQ(lfs_file_close(lfs, &file), 0);
    }
}

// Test single bad block at each position
TEST_P(BadBlockTest, Single) {
    const int NAMEMULT = 64;
    const int FILEMULT = 1;

    // Test bad block at each position (skip first 2 for superblocks initially)
    for (lfs_block_t badblock = 2; badblock < cfg_.block_count; badblock++) {
        // Reset previous bad block
        ASSERT_EQ(lfs_emubd_setwear(&cfg_, badblock - 1, 0), 0);
        // Mark current block as bad
        ASSERT_EQ(lfs_emubd_setwear(&cfg_, badblock, 0xffffffff), 0);

        lfs_t lfs;
        ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        create_content(&lfs, NAMEMULT, FILEMULT);
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
        verify_content(&lfs, NAMEMULT, FILEMULT);
        ASSERT_EQ(lfs_unmount(&lfs), 0);
    }
}

// Test region of bad blocks (causes cascading failures)
TEST_P(BadBlockTest, RegionCorruption) {
    const int NAMEMULT = 64;
    const int FILEMULT = 1;

    // Mark half the blocks as bad (starting from block 2)
    for (lfs_block_t i = 0; i < (cfg_.block_count - 2) / 2; i++) {
        ASSERT_EQ(lfs_emubd_setwear(&cfg_, i + 2, 0xffffffff), 0);
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    create_content(&lfs, NAMEMULT, FILEMULT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    verify_content(&lfs, NAMEMULT, FILEMULT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test alternating bad blocks (causes cascading failures)
TEST_P(BadBlockTest, AlternatingCorruption) {
    const int NAMEMULT = 64;
    const int FILEMULT = 1;

    // Mark every other block as bad (starting from block 2)
    for (lfs_block_t i = 0; i < (cfg_.block_count - 2) / 2; i++) {
        ASSERT_EQ(lfs_emubd_setwear(&cfg_, (2 * i) + 2, 0xffffffff), 0);
    }

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    create_content(&lfs, NAMEMULT, FILEMULT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    verify_content(&lfs, NAMEMULT, FILEMULT);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Test bad superblocks (blocks 0 and 1)
TEST_P(BadBlockTest, Superblocks) {
    // Mark both superblocks as bad
    ASSERT_EQ(lfs_emubd_setwear(&cfg_, 0, 0xffffffff), 0);
    ASSERT_EQ(lfs_emubd_setwear(&cfg_, 1, 0xffffffff), 0);

    lfs_t lfs;
    // Format should fail - no space for superblock
    ASSERT_EQ(lfs_format(&lfs, &cfg_), LFS_ERR_NOSPC);
    // Mount should fail - corrupt superblock
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_CORRUPT);
}

INSTANTIATE_TEST_SUITE_P(
    BadBlocks, BadBlockTest,
    ::testing::ValuesIn(AllBadBlockParams()),
    BadBlockParamNameGenerator{});
