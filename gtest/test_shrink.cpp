/*
 * Filesystem shrink tests
 *
 * Tests shrinking a filesystem using lfs_fs_grow with a smaller block count.
 * Requires LFS_SHRINKNONRELOCATING to be defined.
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

#ifndef LFS_SHRINKNONRELOCATING
// Skip all tests if shrink feature not enabled
class ShrinkTest : public ::testing::Test {
protected:
    void SetUp() override {
        GTEST_SKIP() << "LFS_SHRINKNONRELOCATING not defined";
    }
};

TEST_F(ShrinkTest, Simple) {}
TEST_F(ShrinkTest, Full) {}

#else

// Shrink test parameters
struct ShrinkParams {
    lfs_size_t block_count;
    lfs_size_t after_block_count;

    std::string Name() const {
        char buf[64];
        snprintf(buf, sizeof(buf), "from%u_to%u",
                 (unsigned)block_count, (unsigned)after_block_count);
        return buf;
    }
};

inline std::vector<ShrinkParams> SimpleShrinkParams() {
    std::vector<ShrinkParams> params;
    const lfs_size_t block_counts[] = {10, 15, 20};
    const lfs_size_t after_counts[] = {5, 10, 15, 19};

    for (auto bc : block_counts) {
        for (auto ac : after_counts) {
            if (ac <= bc) {
                params.push_back({bc, ac});
            }
        }
    }
    return params;
}

// Full shrink test parameters (with files)
struct ShrinkFullParams {
    lfs_size_t block_count;
    lfs_size_t after_block_count;
    int files_count;

    std::string Name() const {
        char buf[64];
        snprintf(buf, sizeof(buf), "from%u_to%u_files%d",
                 (unsigned)block_count, (unsigned)after_block_count, files_count);
        return buf;
    }
};

inline std::vector<ShrinkFullParams> FullShrinkParams() {
    std::vector<ShrinkFullParams> params;
    const lfs_size_t block_counts[] = {10, 15, 20};
    const lfs_size_t after_counts[] = {5, 7, 10, 12, 15, 17, 20};
    const int files_counts[] = {7, 8, 9, 10};

    for (auto bc : block_counts) {
        for (auto ac : after_counts) {
            for (auto fc : files_counts) {
                // AFTER_BLOCK_COUNT <= BLOCK_COUNT && FILES_COUNT + 2 < BLOCK_COUNT
                if (ac <= bc && (lfs_size_t)(fc + 2) < bc) {
                    params.push_back({bc, ac, fc});
                }
            }
        }
    }
    return params;
}

// Custom fixture for simple shrink tests
class ShrinkSimpleTest : public LfsTestFixture,
                         public ::testing::WithParamInterface<ShrinkParams> {
protected:
    void SetUp() override {
        // Use the block count from params
        LfsGeometry geom = {"default", 16, 16, 512, GetParam().block_count};
        SetGeometry(geom);
        LfsTestFixture::SetUp();
    }
};

struct ShrinkSimpleNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<ShrinkParams>& info) const {
        return info.param.Name();
    }
};

// Custom fixture for full shrink tests
class ShrinkFullTest : public LfsTestFixture,
                       public ::testing::WithParamInterface<ShrinkFullParams> {
protected:
    void SetUp() override {
        LfsGeometry geom = {"default", 16, 16, 512, GetParam().block_count};
        SetGeometry(geom);
        LfsTestFixture::SetUp();
    }
};

struct ShrinkFullNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<ShrinkFullParams>& info) const {
        return info.param.Name();
    }
};

// Simple shrink test
TEST_P(ShrinkSimpleTest, Simple) {
    const auto& p = GetParam();

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_fs_grow(&lfs, p.after_block_count), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    if (p.block_count != p.after_block_count) {
        // Mount with original config should fail
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_INVAL);
    }

    // Mount with new config should work
    lfs_t lfs2;
    lfs_config cfg2 = cfg_;
    cfg2.block_count = p.after_block_count;
    ASSERT_EQ(lfs_mount(&lfs2, &cfg2), 0);
    ASSERT_EQ(lfs_unmount(&lfs2), 0);
}

// Shrink with files test
TEST_P(ShrinkFullTest, Full) {
    const auto& p = GetParam();
    const lfs_size_t BLOCK_SIZE = cfg_.block_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Create FILES_COUNT + 1 files of BLOCK_SIZE - 0x40 bytes
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    for (int i = 0; i < p.files_count + 1; i++) {
        lfs_file_t file;
        char path[256];
        snprintf(path, sizeof(path), "file_%03d", i);
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);

        std::vector<char> wbuffer(BLOCK_SIZE);
        memset(wbuffer.data(), 'b', BLOCK_SIZE);
        lfs_size_t size = BLOCK_SIZE - 0x40;
        snprintf(wbuffer.data(), wbuffer.size(), "Hi %03d", i);
        ASSERT_EQ(lfs_file_write(&lfs, &file, wbuffer.data(), size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }

    int err = lfs_fs_grow(&lfs, p.after_block_count);

    if (err == 0) {
        // Verify files are still readable
        for (int i = 0; i < p.files_count + 1; i++) {
            lfs_file_t file;
            char path[256];
            snprintf(path, sizeof(path), "file_%03d", i);
            ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);

            lfs_size_t size = BLOCK_SIZE - 0x40;
            std::vector<char> wbuffer(size);
            std::vector<char> wbuffer_ref(size);
            memset(wbuffer_ref.data(), 'b', size);
            snprintf(wbuffer_ref.data(), size, "Hi %03d", i);

            ASSERT_EQ(lfs_file_read(&lfs, &file, wbuffer.data(), BLOCK_SIZE), (lfs_ssize_t)size);
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

            for (lfs_size_t j = 0; j < size; j++) {
                ASSERT_EQ(wbuffer[j], wbuffer_ref[j]) << "Mismatch at byte " << j;
            }
        }
    } else {
        ASSERT_EQ(err, LFS_ERR_NOTEMPTY);
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);

    if (err == 0) {
        if (p.after_block_count != p.block_count) {
            // Mount with original config should fail
            ASSERT_EQ(lfs_mount(&lfs, &cfg_), LFS_ERR_INVAL);
        }

        // Mount with new config and verify
        lfs_t lfs2;
        lfs_config cfg2 = cfg_;
        cfg2.block_count = p.after_block_count;
        ASSERT_EQ(lfs_mount(&lfs2, &cfg2), 0);

        for (int i = 0; i < p.files_count + 1; i++) {
            lfs_file_t file;
            char path[256];
            snprintf(path, sizeof(path), "file_%03d", i);
            ASSERT_EQ(lfs_file_open(&lfs2, &file, path, LFS_O_RDONLY), 0);

            lfs_size_t size = BLOCK_SIZE - 0x40;
            std::vector<char> wbuffer(size);
            std::vector<char> wbuffer_ref(size);
            memset(wbuffer_ref.data(), 'b', size);
            snprintf(wbuffer_ref.data(), size, "Hi %03d", i);

            ASSERT_EQ(lfs_file_read(&lfs2, &file, wbuffer.data(), BLOCK_SIZE), (lfs_ssize_t)size);
            ASSERT_EQ(lfs_file_close(&lfs2, &file), 0);

            for (lfs_size_t j = 0; j < size; j++) {
                ASSERT_EQ(wbuffer[j], wbuffer_ref[j]);
            }
        }
        ASSERT_EQ(lfs_unmount(&lfs2), 0);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Shrink, ShrinkSimpleTest,
    ::testing::ValuesIn(SimpleShrinkParams()),
    ShrinkSimpleNameGenerator{});

INSTANTIATE_TEST_SUITE_P(
    Shrink, ShrinkFullTest,
    ::testing::ValuesIn(FullShrinkParams()),
    ShrinkFullNameGenerator{});

#endif // LFS_SHRINKNONRELOCATING
