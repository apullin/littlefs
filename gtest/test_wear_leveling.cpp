/*
 * Wear leveling benchmark tests
 *
 * Measures erase wear distribution across blocks under different write
 * workloads and block_cycles settings. Produces per-test statistics
 * (min/max/mean/stddev) and histograms to evaluate wear leveling quality.
 *
 * Uses a small, dense filesystem (64 blocks x 512B = 32KB) so that
 * blocks get reused frequently and wear leveling effects are visible.
 */
#include "lfs_test_fixture.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

// Wear leveling parameters: vary block_cycles
struct WearLevelingParams {
    const char* name;
    int32_t block_cycles;
};

inline std::vector<WearLevelingParams> AllWearLevelingParams() {
    return {
        {"disabled",   -1},
        {"cycles_500", 500},
        {"cycles_100", 100},
    };
}

struct WearLevelingParamNameGenerator {
    std::string operator()(
            const ::testing::TestParamInfo<WearLevelingParams>& info) const {
        return info.param.name;
    }
};

// Custom fixture for wear leveling benchmarks
class WearLevelingTest : public LfsTestFixture,
                         public ::testing::WithParamInterface<WearLevelingParams> {
protected:
    void SetUp() override {
        // Small dense filesystem: 64 blocks x 512B = 32KB
        // Small size means blocks get reused often, making wear visible
        LfsGeometry geom = {"small", 16, 16, 512, 64};
        SetGeometry(geom);

        // High erase cycle limit enables wear tracking without blocks going bad
        erase_cycles_ = 1000000;
        block_cycles_ = GetParam().block_cycles;

        LfsTestFixture::SetUp();
    }

    // Collect and print wear statistics for all blocks
    void PrintWearStats(const char* workload) {
        uint32_t min_wear = UINT32_MAX;
        uint32_t max_wear = 0;
        uint64_t total_wear = 0;
        std::vector<uint32_t> wear_counts(cfg_.block_count);

        for (lfs_block_t b = 0; b < cfg_.block_count; b++) {
            uint32_t w = lfs_emubd_wear(&cfg_, b);
            wear_counts[b] = w;
            if (w < min_wear) min_wear = w;
            if (w > max_wear) max_wear = w;
            total_wear += w;
        }

        double mean = (double)total_wear / cfg_.block_count;

        // Standard deviation
        double variance = 0;
        for (lfs_block_t b = 0; b < cfg_.block_count; b++) {
            double diff = (double)wear_counts[b] - mean;
            variance += diff * diff;
        }
        variance /= cfg_.block_count;
        double stddev = sqrt(variance);

        double max_mean_ratio = (mean > 0) ? (double)max_wear / mean : 0;
        double cv = (mean > 0) ? stddev / mean : 0;  // coefficient of variation

        // Print summary
        printf("\n=== Wear Stats: %s (block_cycles=%d) ===\n",
               workload, GetParam().block_cycles);
        printf("  Blocks:     %u\n", cfg_.block_count);
        printf("  Total:      %llu erases\n", (unsigned long long)total_wear);
        printf("  Min:        %u\n", min_wear);
        printf("  Max:        %u\n", max_wear);
        printf("  Mean:       %.1f\n", mean);
        printf("  Stddev:     %.1f\n", stddev);
        printf("  CV:         %.3f (lower = more even)\n", cv);
        printf("  Max/Mean:   %.2fx\n", max_mean_ratio);

        // Histogram (10 buckets)
        if (max_wear > min_wear) {
            const int BUCKETS = 10;
            uint32_t range = max_wear - min_wear;
            uint32_t bucket_size = (range + BUCKETS - 1) / BUCKETS;
            if (bucket_size == 0) bucket_size = 1;
            int histogram[BUCKETS] = {};

            for (lfs_block_t b = 0; b < cfg_.block_count; b++) {
                int bucket = (wear_counts[b] - min_wear) / bucket_size;
                if (bucket >= BUCKETS) bucket = BUCKETS - 1;
                histogram[bucket]++;
            }

            int max_count = *std::max_element(histogram, histogram + BUCKETS);
            printf("  Histogram:\n");
            for (int i = 0; i < BUCKETS; i++) {
                uint32_t lo = min_wear + i * bucket_size;
                uint32_t hi = lo + bucket_size - 1;
                if (i == BUCKETS - 1) hi = max_wear;
                int bar_len = (max_count > 0)
                    ? (histogram[i] * 40 / max_count) : 0;
                printf("    %5u-%-5u | %3d | ", lo, hi, histogram[i]);
                for (int j = 0; j < bar_len; j++) printf("#");
                printf("\n");
            }
        } else {
            printf("  All blocks have identical wear (%u)\n", min_wear);
        }

        // Per-block heatmap (8 blocks per row)
        // Shows spatial distribution: which physical blocks are hot
        printf("  Block heatmap (erases per block, 8/row):\n");
        for (lfs_block_t b = 0; b < cfg_.block_count; b += 8) {
            printf("    %3u: ", b);
            for (lfs_block_t j = b; j < b + 8 && j < cfg_.block_count; j++) {
                // Color-code: mark blocks above 2x mean
                if (mean > 0 && wear_counts[j] > 2.0 * mean) {
                    printf("[%4u] ", wear_counts[j]);
                } else {
                    printf(" %4u  ", wear_counts[j]);
                }
            }
            printf("\n");
        }
        printf("  ([] = above 2x mean)\n\n");

        // Record properties for machine-parseable output
        std::string prefix = std::string(workload) + "_";
        RecordProperty(prefix + "total_erases", (int64_t)total_wear);
        RecordProperty(prefix + "min", min_wear);
        RecordProperty(prefix + "max", max_wear);
        RecordProperty(prefix + "mean", mean);
        RecordProperty(prefix + "stddev", stddev);
        RecordProperty(prefix + "cv", cv);
        RecordProperty(prefix + "max_mean_ratio", max_mean_ratio);

        // Assertions
        EXPECT_GT(total_wear, (uint64_t)0) << "Workload produced no erases";
    }
};

// --- Workload 1: Sequential Rewrite ---
// Overwrite the same file 2000 times. Worst case for naive allocation:
// without wear leveling, only a few blocks get hammered.
TEST_P(WearLevelingTest, SequentialRewrite) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    uint8_t buf[64];
    memset(buf, 0xAA, sizeof(buf));

    for (int i = 0; i < 2000; i++) {
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, "rewrite",
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);
        // Write ~256B per iteration
        for (int j = 0; j < 4; j++) {
            ASSERT_EQ(lfs_file_write(&lfs, &file, buf, sizeof(buf)),
                      (lfs_ssize_t)sizeof(buf));
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
    PrintWearStats("sequential_rewrite");
}

// --- Workload 2: Many Small Files ---
// Create/delete/recreate small files in many rounds. Heavy metadata churn.
TEST_P(WearLevelingTest, ManySmallFiles) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    const int FILES_PER_ROUND = 20;
    const int ROUNDS = 500;
    char path[64];
    uint8_t data[32];
    memset(data, 0xBB, sizeof(data));

    for (int round = 0; round < ROUNDS; round++) {
        // Create files
        for (int i = 0; i < FILES_PER_ROUND; i++) {
            snprintf(path, sizeof(path), "f%d", i);
            lfs_file_t file;
            ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);
            ASSERT_EQ(lfs_file_write(&lfs, &file, data, sizeof(data)),
                      (lfs_ssize_t)sizeof(data));
            ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        }

        // Delete all files
        for (int i = 0; i < FILES_PER_ROUND; i++) {
            snprintf(path, sizeof(path), "f%d", i);
            ASSERT_EQ(lfs_remove(&lfs, path), 0);
        }
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
    PrintWearStats("many_small_files");
}

// --- Workload 3: Random Sized Writes ---
// PRNG-driven file creation with deletion when full. Realistic mixed workload.
TEST_P(WearLevelingTest, RandomSizedWrites) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    uint32_t prng = 42;
    auto next_rand = [&prng]() -> uint32_t {
        prng = prng * 1103515245 + 12345;
        return prng;
    };

    const int ITERATIONS = 2000;
    int file_counter = 0;
    int oldest_file = 0;
    int live_files = 0;

    for (int i = 0; i < ITERATIONS; i++) {
        // Delete oldest files when we have too many (keep ~8 live)
        while (live_files > 8) {
            char path[64];
            snprintf(path, sizeof(path), "r%d", oldest_file);
            lfs_remove(&lfs, path);  // ignore errors if already gone
            oldest_file++;
            live_files--;
        }

        // Create a file with random size (32B to 512B)
        char path[64];
        snprintf(path, sizeof(path), "r%d", file_counter);
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, path,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), 0);

        uint32_t size = 32 + (next_rand() % (512 - 32));
        uint8_t buf[64];
        uint32_t written = 0;
        while (written < size) {
            uint32_t chunk = std::min((uint32_t)sizeof(buf), size - written);
            memset(buf, (uint8_t)(file_counter & 0xFF), chunk);
            lfs_ssize_t res = lfs_file_write(&lfs, &file, buf, chunk);
            if (res < 0) break;  // NOSPC is ok
            written += res;
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
        file_counter++;
        live_files++;
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
    PrintWearStats("random_sized_writes");
}

// --- Workload 4: Append Log ---
// Append to a log file, truncate and restart when full. Common embedded pattern.
TEST_P(WearLevelingTest, AppendLog) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    uint8_t entry[64];
    memset(entry, 0xCC, sizeof(entry));
    const lfs_soff_t MAX_LOG_SIZE = 8192;  // 8KB before truncate (~16 blocks)
    const int CYCLES = 200;

    for (int cycle = 0; cycle < CYCLES; cycle++) {
        // Append until log is full
        lfs_file_t file;
        ASSERT_EQ(lfs_file_open(&lfs, &file, "log",
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND), 0);

        lfs_soff_t size = lfs_file_seek(&lfs, &file, 0, LFS_SEEK_END);

        while (size < MAX_LOG_SIZE) {
            lfs_ssize_t res = lfs_file_write(&lfs, &file, entry, sizeof(entry));
            if (res < 0) break;
            size += res;
        }
        ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

        // Truncate log (delete and recreate)
        ASSERT_EQ(lfs_remove(&lfs, "log"), 0);
    }

    ASSERT_EQ(lfs_unmount(&lfs), 0);
    PrintWearStats("append_log");
}

INSTANTIATE_TEST_SUITE_P(
    WearLeveling, WearLevelingTest,
    ::testing::ValuesIn(AllWearLevelingParams()),
    WearLevelingParamNameGenerator{});
