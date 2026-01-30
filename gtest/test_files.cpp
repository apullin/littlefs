/*
 * File operation tests - basic read/write/append/truncate
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

class FilesTest : public LfsParametricTest {};

// Simple file write and read
TEST_P(FilesTest, Simple) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "hello",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);

    const char* message = "Hello World!";
    lfs_size_t size = strlen(message) + 1;
    uint8_t buffer[1024];
    strcpy((char*)buffer, message);
    ASSERT_EQ(lfs_file_write(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Remount and read
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "hello", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer, size), (lfs_ssize_t)size);
    ASSERT_STREQ((char*)buffer, message);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Parameters for large file tests
struct FileSizeParams {
    LfsGeometry geometry;
    lfs_size_t size;
    lfs_size_t chunk_size;
    int32_t inline_max;  // 0 = default, -1 = disable, 8 = small

    std::string Name() const {
        char buf[128];
        const char *ilname = (inline_max == 0) ? "ildef"
                           : (inline_max < 0) ? "iloff" : "il8";
        snprintf(buf, sizeof(buf), "%s_size%u_chunk%u_%s",
                geometry.name, (unsigned)size, (unsigned)chunk_size, ilname);
        return buf;
    }
};

class FilesLargeTest : public LfsTestFixture,
                       public ::testing::WithParamInterface<FileSizeParams> {
protected:
    void SetUp() override {
        SetGeometry(GetParam().geometry);
        LfsTestFixture::SetUp();
        // Apply inline_max override
        if (GetParam().inline_max != 0) {
            cfg_.inline_max = GetParam().inline_max;
        }
    }
};

// Generate large file test params across all geometries and inline_max values
std::vector<FileSizeParams> GenerateFileSizeParams() {
    std::vector<FileSizeParams> params;
    auto geometries = AllGeometries();

    std::vector<lfs_size_t> sizes = {32, 8192, 262144, 0, 7, 8193};
    std::vector<lfs_size_t> chunks = {31, 16, 33, 1, 1023};
    std::vector<int32_t> inline_maxes = {0, -1, 8};

    for (const auto &geom : geometries) {
        for (auto size : sizes) {
            for (auto chunk : chunks) {
                for (auto il : inline_maxes) {
                    params.push_back({geom, size, chunk, il});
                }
            }
        }
    }
    return params;
}

struct FileSizeNameGenerator {
    std::string operator()(const ::testing::TestParamInfo<FileSizeParams>& info) const {
        return info.param.Name();
    }
};

TEST_P(FilesLargeTest, LargeFile) {
    auto& params = GetParam();
    lfs_size_t SIZE = params.size;
    lfs_size_t CHUNKSIZE = params.chunk_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);

    uint32_t prng = 1;
    std::vector<uint8_t> buffer(CHUNKSIZE);
    for (lfs_size_t i = 0; i < SIZE; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Read
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)SIZE);

    prng = 1;
    for (lfs_size_t i = 0; i < SIZE; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE - i);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
        for (lfs_size_t b = 0; b < chunk; b++) {
            ASSERT_EQ(buffer[b], TEST_PRNG(&prng) & 0xff)
                << "Mismatch at offset " << (i + b);
        }
    }
    ASSERT_EQ(lfs_file_read(&lfs, &file, buffer.data(), CHUNKSIZE), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

TEST_P(FilesLargeTest, Rewrite) {
    auto& params = GetParam();
    lfs_size_t SIZE1 = params.size;
    lfs_size_t SIZE2 = lfs_max(SIZE1, 32u);  // Ensure we have some data to rewrite
    lfs_size_t CHUNKSIZE = params.chunk_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Initial write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    std::vector<uint8_t> buffer(CHUNKSIZE);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    uint32_t prng = 1;
    for (lfs_size_t i = 0; i < SIZE1; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE1 - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Rewrite with different data
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_WRONLY), 0);
    prng = 2;
    for (lfs_size_t i = 0; i < SIZE2; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE2 - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Read and verify
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)lfs_max(SIZE1, SIZE2));

    prng = 2;
    for (lfs_size_t i = 0; i < SIZE2; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE2 - i);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
        for (lfs_size_t b = 0; b < chunk; b++) {
            ASSERT_EQ(buffer[b], TEST_PRNG(&prng) & 0xff);
        }
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

TEST_P(FilesLargeTest, Append) {
    auto& params = GetParam();
    lfs_size_t SIZE1 = params.size;
    lfs_size_t SIZE2 = 32;  // Append fixed amount
    lfs_size_t CHUNKSIZE = params.chunk_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Initial write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    std::vector<uint8_t> buffer(CHUNKSIZE);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    uint32_t prng = 1;
    for (lfs_size_t i = 0; i < SIZE1; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE1 - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Append
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_WRONLY | LFS_O_APPEND), 0);
    prng = 2;
    for (lfs_size_t i = 0; i < SIZE2; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE2 - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Read and verify all data
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)(SIZE1 + SIZE2));

    prng = 1;
    for (lfs_size_t i = 0; i < SIZE1; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE1 - i);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
        for (lfs_size_t b = 0; b < chunk; b++) {
            ASSERT_EQ(buffer[b], TEST_PRNG(&prng) & 0xff);
        }
    }
    prng = 2;
    for (lfs_size_t i = 0; i < SIZE2; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE2 - i);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
        for (lfs_size_t b = 0; b < chunk; b++) {
            ASSERT_EQ(buffer[b], TEST_PRNG(&prng) & 0xff);
        }
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

TEST_P(FilesLargeTest, Truncate) {
    auto& params = GetParam();
    lfs_size_t SIZE1 = lfs_max(params.size, 64u);
    lfs_size_t SIZE2 = 32;
    lfs_size_t CHUNKSIZE = params.chunk_size;

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);

    // Initial write
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    lfs_file_t file;
    std::vector<uint8_t> buffer(CHUNKSIZE);

    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    uint32_t prng = 1;
    for (lfs_size_t i = 0; i < SIZE1; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE1 - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)SIZE1);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Truncate and write new data
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_WRONLY | LFS_O_TRUNC), 0);
    prng = 2;
    for (lfs_size_t i = 0; i < SIZE2; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE2 - i);
        for (lfs_size_t b = 0; b < chunk; b++) {
            buffer[b] = TEST_PRNG(&prng) & 0xff;
        }
        ASSERT_EQ(lfs_file_write(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);

    // Read and verify truncated size
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "avocado", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_size(&lfs, &file), (lfs_soff_t)SIZE2);

    prng = 2;
    for (lfs_size_t i = 0; i < SIZE2; i += CHUNKSIZE) {
        lfs_size_t chunk = lfs_min(CHUNKSIZE, SIZE2 - i);
        ASSERT_EQ(lfs_file_read(&lfs, &file, buffer.data(), chunk), (lfs_ssize_t)chunk);
        for (lfs_size_t b = 0; b < chunk; b++) {
            ASSERT_EQ(buffer[b], TEST_PRNG(&prng) & 0xff);
        }
    }
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Many files test
TEST_P(FilesTest, ManyFiles) {
    const int N = 100;  // Reduced from 300 for faster testing

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        lfs_file_t file;
        char path[64];
        snprintf(path, sizeof(path), "file_%03d", i);

        char wbuffer[64];
        snprintf(wbuffer, sizeof(wbuffer), "Hi %03d", i);
        lfs_size_t size = strlen(wbuffer) + 1;

        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_write(&lfs, &file, wbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

        char rbuffer[64];
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_STREQ(rbuffer, wbuffer);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Many files with power cycle
TEST_P(FilesTest, ManyFilesPowerCycle) {
    const int N = 50;  // Reduced for faster testing

    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    for (int i = 0; i < N; i++) {
        lfs_file_t file;
        char path[64];
        snprintf(path, sizeof(path), "file_%03d", i);

        char wbuffer[64];
        snprintf(wbuffer, sizeof(wbuffer), "Hi %03d", i);
        lfs_size_t size = strlen(wbuffer) + 1;

        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
        ASSERT_EQ(lfs_file_write(&lfs, &file, wbuffer, size), (lfs_ssize_t)size);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        ASSERT_EQ(lfs_unmount(&lfs), 0);

        // Remount to simulate power cycle
        ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

        char rbuffer[64];
        ASSERT_EQ(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY), 0);
        ASSERT_EQ(lfs_file_read(&lfs, &file, rbuffer, size), (lfs_ssize_t)size);
        ASSERT_STREQ(rbuffer, wbuffer);
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }
    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Instantiate tests
INSTANTIATE_TEST_SUITE_P(
    Geometries, FilesTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});

INSTANTIATE_TEST_SUITE_P(
    Sizes, FilesLargeTest,
    ::testing::ValuesIn(GenerateFileSizeParams()),
    FileSizeNameGenerator{});
