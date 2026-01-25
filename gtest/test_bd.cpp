// Block device tests - validates the underlying block device is working
// These don't test littlefs itself, just the emulated block device.
//
// Note: We use 251, a prime, in places to avoid aliasing powers of 2.

#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <algorithm>
#include <cstring>

// Parameters for block device tests
struct BdTestParams {
    LfsGeometry geometry;
    bool use_block_size_for_read;  // true = use block_size, false = use read_size
    bool use_block_size_for_prog;  // true = use block_size, false = use prog_size

    std::string Name() const {
        std::string name = geometry.name;
        name += use_block_size_for_read ? "_readBlock" : "_readSize";
        name += use_block_size_for_prog ? "_progBlock" : "_progSize";
        return name;
    }
};

class BdTest : public LfsTestFixture,
               public ::testing::WithParamInterface<BdTestParams> {
protected:
    void SetUp() override {
        SetGeometry(GetParam().geometry);
        LfsTestFixture::SetUp();
    }

    lfs_size_t read_size() const {
        return GetParam().use_block_size_for_read
            ? cfg_.block_size
            : cfg_.read_size;
    }

    lfs_size_t prog_size() const {
        return GetParam().use_block_size_for_prog
            ? cfg_.block_size
            : cfg_.prog_size;
    }
};

// Generate all parameter combinations
std::vector<BdTestParams> GenerateBdTestParams() {
    std::vector<BdTestParams> params;
    for (const auto& geom : AllGeometries()) {
        // READ x PROG = 2x2 = 4 combinations per geometry
        for (bool read_block : {false, true}) {
            for (bool prog_block : {false, true}) {
                params.push_back({geom, read_block, prog_block});
            }
        }
    }
    return params;
}

struct BdTestNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<BdTestParams>& info) const {
        return info.param.Name();
    }
};

// Test: Write and read a single block
TEST_P(BdTest, OneBlock) {
    const lfs_size_t READ = read_size();
    const lfs_size_t PROG = prog_size();
    std::vector<uint8_t> buffer(std::max(READ, PROG));

    // Write data
    LFS_ASSERT_OK(cfg_.erase(&cfg_, 0));
    for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
        for (lfs_off_t j = 0; j < PROG; j++) {
            buffer[j] = (i + j) % 251;
        }
        LFS_ASSERT_OK(cfg_.prog(&cfg_, 0, i, buffer.data(), PROG));
    }

    // Read data
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, 0, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (i + j) % 251)
                << "Mismatch at offset " << (i + j);
        }
    }
}

// Test: Write and read two blocks, verify no interference
TEST_P(BdTest, TwoBlock) {
    const lfs_size_t READ = read_size();
    const lfs_size_t PROG = prog_size();
    std::vector<uint8_t> buffer(std::max(READ, PROG));

    // Write block 0
    lfs_block_t block = 0;
    LFS_ASSERT_OK(cfg_.erase(&cfg_, block));
    for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
        for (lfs_off_t j = 0; j < PROG; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_ASSERT_OK(cfg_.prog(&cfg_, block, i, buffer.data(), PROG));
    }

    // Read block 0
    block = 0;
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (block + i + j) % 251);
        }
    }

    // Write block 1
    block = 1;
    LFS_ASSERT_OK(cfg_.erase(&cfg_, block));
    for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
        for (lfs_off_t j = 0; j < PROG; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_ASSERT_OK(cfg_.prog(&cfg_, block, i, buffer.data(), PROG));
    }

    // Read block 1
    block = 1;
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (block + i + j) % 251);
        }
    }

    // Read block 0 again - verify it wasn't corrupted
    block = 0;
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (block + i + j) % 251);
        }
    }
}

// Test: Write first and last blocks
TEST_P(BdTest, LastBlock) {
    const lfs_size_t READ = read_size();
    const lfs_size_t PROG = prog_size();
    std::vector<uint8_t> buffer(std::max(READ, PROG));

    // Write block 0
    lfs_block_t block = 0;
    LFS_ASSERT_OK(cfg_.erase(&cfg_, block));
    for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
        for (lfs_off_t j = 0; j < PROG; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_ASSERT_OK(cfg_.prog(&cfg_, block, i, buffer.data(), PROG));
    }

    // Read block 0
    block = 0;
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (block + i + j) % 251);
        }
    }

    // Write block n-1
    block = cfg_.block_count - 1;
    LFS_ASSERT_OK(cfg_.erase(&cfg_, block));
    for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
        for (lfs_off_t j = 0; j < PROG; j++) {
            buffer[j] = (block + i + j) % 251;
        }
        LFS_ASSERT_OK(cfg_.prog(&cfg_, block, i, buffer.data(), PROG));
    }

    // Read block n-1
    block = cfg_.block_count - 1;
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (block + i + j) % 251);
        }
    }

    // Read block 0 again
    block = 0;
    for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
        LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
        for (lfs_off_t j = 0; j < READ; j++) {
            ASSERT_EQ(buffer[j], (block + i + j) % 251);
        }
    }
}

// Test: Write and read blocks at powers of two
TEST_P(BdTest, PowersOfTwo) {
    const lfs_size_t READ = read_size();
    const lfs_size_t PROG = prog_size();
    std::vector<uint8_t> buffer(std::max(READ, PROG));

    // Write/read every power of 2
    lfs_block_t block = 1;
    while (block < cfg_.block_count) {
        // Write
        LFS_ASSERT_OK(cfg_.erase(&cfg_, block));
        for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
            for (lfs_off_t j = 0; j < PROG; j++) {
                buffer[j] = (block + i + j) % 251;
            }
            LFS_ASSERT_OK(cfg_.prog(&cfg_, block, i, buffer.data(), PROG));
        }

        // Read
        for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
            LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
            for (lfs_off_t j = 0; j < READ; j++) {
                ASSERT_EQ(buffer[j], (block + i + j) % 251);
            }
        }

        block *= 2;
    }

    // Read every power of 2 again
    block = 1;
    while (block < cfg_.block_count) {
        for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
            LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
            for (lfs_off_t j = 0; j < READ; j++) {
                ASSERT_EQ(buffer[j], (block + i + j) % 251);
            }
        }
        block *= 2;
    }
}

// Test: Write and read blocks at Fibonacci numbers
TEST_P(BdTest, Fibonacci) {
    const lfs_size_t READ = read_size();
    const lfs_size_t PROG = prog_size();
    std::vector<uint8_t> buffer(std::max(READ, PROG));

    // Write/read every Fibonacci number on our device
    lfs_block_t block = 1;
    lfs_block_t block_prev = 1;
    while (block < cfg_.block_count) {
        // Write
        LFS_ASSERT_OK(cfg_.erase(&cfg_, block));
        for (lfs_off_t i = 0; i < cfg_.block_size; i += PROG) {
            for (lfs_off_t j = 0; j < PROG; j++) {
                buffer[j] = (block + i + j) % 251;
            }
            LFS_ASSERT_OK(cfg_.prog(&cfg_, block, i, buffer.data(), PROG));
        }

        // Read
        for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
            LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
            for (lfs_off_t j = 0; j < READ; j++) {
                ASSERT_EQ(buffer[j], (block + i + j) % 251);
            }
        }

        lfs_block_t next_block = block + block_prev;
        block_prev = block;
        block = next_block;
    }

    // Read every Fibonacci number again
    block = 1;
    block_prev = 1;
    while (block < cfg_.block_count) {
        for (lfs_off_t i = 0; i < cfg_.block_size; i += READ) {
            LFS_ASSERT_OK(cfg_.read(&cfg_, block, i, buffer.data(), READ));
            for (lfs_off_t j = 0; j < READ; j++) {
                ASSERT_EQ(buffer[j], (block + i + j) % 251);
            }
        }

        lfs_block_t next_block = block + block_prev;
        block_prev = block;
        block = next_block;
    }
}

INSTANTIATE_TEST_SUITE_P(
    BlockDevice,
    BdTest,
    ::testing::ValuesIn(GenerateBdTestParams()),
    BdTestNameGenerator{}
);
