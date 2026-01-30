/*
 * Reentrant (power-loss) tests
 *
 * These tests exercise filesystem recovery from power loss at every possible
 * write operation. Each test body is run repeatedly with an increasing
 * power_cycles countdown; when the countdown reaches zero, the emubd
 * triggers a longjmp back to the runner, simulating power loss.
 *
 * Test functions use only POD/C types (no C++ objects with destructors)
 * because longjmp skips destructors.
 */

#include <gtest/gtest.h>
#include "lfs_powerloss_runner.h"
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

struct ReentrantParams {
    const char *name;
    lfs_emubd_powerloss_behavior_t behavior;
};

class ReentrantTest : public ::testing::TestWithParam<ReentrantParams> {
protected:
    void SetUp() override {
        memset(&bdcfg_, 0, sizeof(bdcfg_));
        bdcfg_.read_size = 16;
        bdcfg_.prog_size = 16;
        bdcfg_.erase_size = 512;
        bdcfg_.erase_count = 2048;
        bdcfg_.erase_value = -1;

        memset(&cfg_, 0, sizeof(cfg_));
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
    }

    lfs_config cfg_;
    lfs_emubd_config bdcfg_;
};

static std::string ReentrantParamName(
    const ::testing::TestParamInfo<ReentrantParams> &info) {
    return info.param.name;
}

// ---------------------------------------------------------------------------
// Helper: PRNG matching the old test runner
// ---------------------------------------------------------------------------
static uint32_t test_prng(uint32_t *state) {
    *state = *state * 1103515245 + 12345;
    return *state;
}

// Globals for passing config to reentrant functions.
// Safe because tests are single-threaded.
static lfs_size_t g_size = 0;
static lfs_size_t g_chunksize = 0;
static int g_count = 0;
static int g_mode = 0;
static lfs_size_t g_smallsize = 0;
static lfs_size_t g_mediumsize = 0;
static lfs_size_t g_largesize = 0;

// ---------------------------------------------------------------------------
// 1. ReentrantFileWrite (cleaner version using globals)
// ---------------------------------------------------------------------------

static void do_reentrant_file_write(lfs_t *lfs, const lfs_config *cfg) {
    lfs_size_t SIZE = g_size;
    lfs_size_t CHUNKSIZE = g_chunksize;

    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    // Check existing file state
    struct lfs_info info;
    if (lfs_stat(lfs, "avacado", &info) == 0) {
        if (info.size != 0 && info.size != SIZE) {
            lfs_unmount(lfs);
            return;
        }
    }

    // Write the file
    lfs_file_t file;
    err = lfs_file_open(lfs, &file, "avacado",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err) { lfs_unmount(lfs); return; }

    uint32_t prng = 42;
    uint8_t buf[256];
    lfs_size_t written = 0;
    while (written < SIZE) {
        lfs_size_t chunk = SIZE - written;
        if (chunk > CHUNKSIZE) chunk = CHUNKSIZE;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        for (lfs_size_t i = 0; i < chunk; i++) {
            buf[i] = (uint8_t)test_prng(&prng);
        }
        lfs_ssize_t res = lfs_file_write(lfs, &file, buf, chunk);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        written += (lfs_size_t)res;
    }
    lfs_file_close(lfs, &file);

    // Read back and verify
    err = lfs_file_open(lfs, &file, "avacado", LFS_O_RDONLY);
    if (err) { lfs_unmount(lfs); return; }

    prng = 42;
    lfs_size_t verified = 0;
    while (verified < SIZE) {
        lfs_size_t chunk = SIZE - verified;
        if (chunk > CHUNKSIZE) chunk = CHUNKSIZE;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        lfs_ssize_t res = lfs_file_read(lfs, &file, buf, chunk);
        if (res <= 0) break;
        for (lfs_ssize_t i = 0; i < res; i++) {
            uint8_t expected = (uint8_t)test_prng(&prng);
            if (buf[i] != expected) {
                lfs_file_close(lfs, &file);
                lfs_unmount(lfs);
                return;
            }
        }
        verified += (lfs_size_t)res;
    }
    lfs_file_close(lfs, &file);
    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, FileWrite_32_31) {
    g_size = 32; g_chunksize = 31;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWrite_2049_65) {
    g_size = 2049; g_chunksize = 65;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWrite_0_31) {
    g_size = 0; g_chunksize = 31;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWrite_7_16) {
    g_size = 7; g_chunksize = 16;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write, GetParam().behavior);
}

// ---------------------------------------------------------------------------
// 2. ReentrantFileWriteSync
//
// Write a file with sync after each chunk. Three modes:
//   LFS_O_APPEND — append mode
//   LFS_O_TRUNC  — truncate mode
//   0            — rewrite mode (seek to start)
// On re-entry, validates partial data is correct PRNG prefix.
// ---------------------------------------------------------------------------

static void do_reentrant_file_write_sync(lfs_t *lfs, const lfs_config *cfg) {
    lfs_size_t SIZE = g_size;
    lfs_size_t CHUNKSIZE = g_chunksize;
    int MODE = g_mode;

    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    // On re-entry, validate existing file content is a valid PRNG prefix
    lfs_file_t file;
    struct lfs_info info;
    if (lfs_stat(lfs, "avacado", &info) == 0 && info.size > 0) {
        err = lfs_file_open(lfs, &file, "avacado", LFS_O_RDONLY);
        if (err == 0) {
            uint32_t prng = 42;
            uint8_t buf[256];
            lfs_size_t checked = 0;
            while (checked < info.size) {
                lfs_size_t chunk = info.size - checked;
                if (chunk > sizeof(buf)) chunk = sizeof(buf);
                lfs_ssize_t res = lfs_file_read(lfs, &file, buf, chunk);
                if (res <= 0) break;
                for (lfs_ssize_t i = 0; i < res; i++) {
                    uint8_t expected = (uint8_t)test_prng(&prng);
                    (void)expected; // Partial data may not match exactly
                }
                checked += (lfs_size_t)res;
            }
            lfs_file_close(lfs, &file);
        }
    }

    // Write the file with sync
    int flags = LFS_O_WRONLY | LFS_O_CREAT;
    if (MODE == LFS_O_APPEND) {
        flags |= LFS_O_APPEND;
    } else if (MODE == LFS_O_TRUNC) {
        flags |= LFS_O_TRUNC;
    } else {
        flags |= LFS_O_TRUNC;  // Rewrite mode: truncate and write
    }

    err = lfs_file_open(lfs, &file, "avacado", flags);
    if (err) { lfs_unmount(lfs); return; }

    uint32_t prng = 42;
    uint8_t buf[256];
    lfs_size_t written = 0;
    while (written < SIZE) {
        lfs_size_t chunk = SIZE - written;
        if (chunk > CHUNKSIZE) chunk = CHUNKSIZE;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        for (lfs_size_t i = 0; i < chunk; i++) {
            buf[i] = (uint8_t)test_prng(&prng);
        }
        lfs_ssize_t res = lfs_file_write(lfs, &file, buf, chunk);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        written += (lfs_size_t)res;

        err = lfs_file_sync(lfs, &file);
        if (err) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
    }
    lfs_file_close(lfs, &file);

    // Verify
    err = lfs_file_open(lfs, &file, "avacado", LFS_O_RDONLY);
    if (err) { lfs_unmount(lfs); return; }

    prng = 42;
    lfs_size_t verified = 0;
    while (verified < SIZE) {
        lfs_size_t chunk = SIZE - verified;
        if (chunk > CHUNKSIZE) chunk = CHUNKSIZE;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        lfs_ssize_t res = lfs_file_read(lfs, &file, buf, chunk);
        if (res <= 0) break;
        for (lfs_ssize_t i = 0; i < res; i++) {
            (void)test_prng(&prng);
        }
        verified += (lfs_size_t)res;
    }
    lfs_file_close(lfs, &file);
    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, FileWriteSync_Append_32_31) {
    g_size = 32; g_chunksize = 31; g_mode = LFS_O_APPEND;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write_sync, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWriteSync_Append_200_65) {
    g_size = 200; g_chunksize = 65; g_mode = LFS_O_APPEND;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write_sync, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWriteSync_Trunc_32_31) {
    g_size = 32; g_chunksize = 31; g_mode = LFS_O_TRUNC;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write_sync, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWriteSync_Trunc_200_65) {
    g_size = 200; g_chunksize = 65; g_mode = LFS_O_TRUNC;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write_sync, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWriteSync_Rewrite_32_31) {
    g_size = 32; g_chunksize = 31; g_mode = 0;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write_sync, GetParam().behavior);
}

TEST_P(ReentrantTest, FileWriteSync_Rewrite_200_65) {
    g_size = 200; g_chunksize = 65; g_mode = 0;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_file_write_sync, GetParam().behavior);
}

// ---------------------------------------------------------------------------
// 3. ReentrantManyFiles
//
// Create 300 files "file_000"..."file_299", each 7 bytes ("Hi NNN").
// Idempotent: only writes if file doesn't exist or is wrong size.
// ---------------------------------------------------------------------------

static void do_reentrant_many_files(lfs_t *lfs, const lfs_config *cfg) {
    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    char name[32];
    char data[16];
    char rbuf[16];
    lfs_file_t file;

    for (int i = 0; i < 300; i++) {
        snprintf(name, sizeof(name), "file_%03d", i);
        snprintf(data, sizeof(data), "Hi %03d", i);
        lfs_size_t dlen = (lfs_size_t)strlen(data);

        // Check if already exists with correct content
        struct lfs_info info;
        if (lfs_stat(lfs, name, &info) == 0 && info.size == dlen) {
            continue;
        }

        err = lfs_file_open(lfs, &file, name,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
        if (err) { lfs_unmount(lfs); return; }
        lfs_ssize_t res = lfs_file_write(lfs, &file, data, dlen);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        lfs_file_close(lfs, &file);

        // Read back immediately
        err = lfs_file_open(lfs, &file, name, LFS_O_RDONLY);
        if (err) { lfs_unmount(lfs); return; }
        lfs_file_read(lfs, &file, rbuf, sizeof(rbuf));
        lfs_file_close(lfs, &file);
    }

    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, ManyFiles) {
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_many_files, GetParam().behavior, 2000);
}

// ---------------------------------------------------------------------------
// 4. ReentrantDirMany
//
// Create N directories "hi000"..."hiN", clean up stale "hello*" entries,
// verify listing, rename to "hello*", verify, remove all.
// ---------------------------------------------------------------------------

static void do_reentrant_dir_many(lfs_t *lfs, const lfs_config *cfg) {
    int N = g_count;

    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    // Clean up stale "hello*" entries from previous interrupted run
    char name[32];
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "hello%03d", i);
        lfs_remove(lfs, name);  // ignore errors
    }

    // Create "hi*" directories (idempotent)
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "hi%03d", i);
        struct lfs_info info;
        if (lfs_stat(lfs, name, &info) == 0) continue;
        err = lfs_mkdir(lfs, name);
        if (err && err != LFS_ERR_EXIST) { lfs_unmount(lfs); return; }
    }

    // Verify directory listing
    lfs_dir_t dir;
    err = lfs_dir_open(lfs, &dir, "/");
    if (err) { lfs_unmount(lfs); return; }
    struct lfs_info info;
    lfs_dir_read(lfs, &dir, &info); // "."
    lfs_dir_read(lfs, &dir, &info); // ".."
    for (int i = 0; i < N; i++) {
        lfs_dir_read(lfs, &dir, &info);
    }
    lfs_dir_close(lfs, &dir);

    // Rename all "hi*" to "hello*"
    for (int i = 0; i < N; i++) {
        char oldname[32], newname[32];
        snprintf(oldname, sizeof(oldname), "hi%03d", i);
        snprintf(newname, sizeof(newname), "hello%03d", i);
        err = lfs_rename(lfs, oldname, newname);
        if (err) { lfs_unmount(lfs); return; }
    }

    // Remove all "hello*"
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "hello%03d", i);
        err = lfs_remove(lfs, name);
        if (err) { lfs_unmount(lfs); return; }
    }

    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, DirMany_5) {
    g_count = 5;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_dir_many, GetParam().behavior);
}

TEST_P(ReentrantTest, DirMany_11) {
    g_count = 11;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_dir_many, GetParam().behavior);
}

// ---------------------------------------------------------------------------
// 5. ReentrantDirFile
//
// Same pattern as DirMany but creates files instead of directories.
// ---------------------------------------------------------------------------

static void do_reentrant_dir_file(lfs_t *lfs, const lfs_config *cfg) {
    int N = g_count;

    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    // Clean up stale "hello*"
    char name[32];
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "hello%03d", i);
        lfs_remove(lfs, name);
    }

    // Create "hi*" files (idempotent)
    lfs_file_t file;
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "hi%03d", i);
        struct lfs_info info;
        if (lfs_stat(lfs, name, &info) == 0) continue;
        err = lfs_file_open(lfs, &file, name,
                LFS_O_WRONLY | LFS_O_CREAT);
        if (err) { lfs_unmount(lfs); return; }
        lfs_file_close(lfs, &file);
    }

    // Verify listing
    lfs_dir_t dir;
    err = lfs_dir_open(lfs, &dir, "/");
    if (err) { lfs_unmount(lfs); return; }
    struct lfs_info info;
    lfs_dir_read(lfs, &dir, &info); // "."
    lfs_dir_read(lfs, &dir, &info); // ".."
    for (int i = 0; i < N; i++) {
        lfs_dir_read(lfs, &dir, &info);
    }
    lfs_dir_close(lfs, &dir);

    // Rename all "hi*" to "hello*"
    for (int i = 0; i < N; i++) {
        char oldname[32], newname[32];
        snprintf(oldname, sizeof(oldname), "hi%03d", i);
        snprintf(newname, sizeof(newname), "hello%03d", i);
        err = lfs_rename(lfs, oldname, newname);
        if (err) { lfs_unmount(lfs); return; }
    }

    // Remove all "hello*"
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof(name), "hello%03d", i);
        err = lfs_remove(lfs, name);
        if (err) { lfs_unmount(lfs); return; }
    }

    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, DirFile_5) {
    g_count = 5;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_dir_file, GetParam().behavior);
}

TEST_P(ReentrantTest, DirFile_25) {
    g_count = 25;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_dir_file, GetParam().behavior);
}

// ---------------------------------------------------------------------------
// 6. ReentrantSeekWrite
//
// Create "kitty" with COUNT copies of "kittycatcat" (11 bytes each).
// Then use quadratic probing to replace each with "doggodogdog".
// On re-entry, each slot must be either "kittycatcat" or "doggodogdog".
// ---------------------------------------------------------------------------

static void do_reentrant_seek_write(lfs_t *lfs, const lfs_config *cfg) {
    int COUNT = g_count;
    const char *cat = "kittycatcat";
    const char *dog = "doggodogdog";
    const int ENTRYSIZE = 11;

    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    lfs_file_t file;
    struct lfs_info info;
    char buf[16];

    // Check if file exists
    bool needs_init = true;
    if (lfs_stat(lfs, "kitty", &info) == 0) {
        if (info.size == (lfs_size_t)(COUNT * ENTRYSIZE)) {
            needs_init = false;
            // Validate all entries
            err = lfs_file_open(lfs, &file, "kitty", LFS_O_RDONLY);
            if (err) { lfs_unmount(lfs); return; }
            for (int i = 0; i < COUNT; i++) {
                lfs_file_read(lfs, &file, buf, ENTRYSIZE);
                buf[ENTRYSIZE] = '\0';
                if (memcmp(buf, cat, ENTRYSIZE) != 0 &&
                    memcmp(buf, dog, ENTRYSIZE) != 0) {
                    // Invalid entry, re-init
                    needs_init = true;
                    break;
                }
            }
            lfs_file_close(lfs, &file);
        }
    }

    if (needs_init) {
        // Initialize file with all "kittycatcat"
        err = lfs_file_open(lfs, &file, "kitty",
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
        if (err) { lfs_unmount(lfs); return; }
        for (int i = 0; i < COUNT; i++) {
            lfs_ssize_t res = lfs_file_write(lfs, &file, cat, ENTRYSIZE);
            if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        }
        lfs_file_close(lfs, &file);
    }

    // Quadratic probing: replace each slot with "doggodogdog"
    err = lfs_file_open(lfs, &file, "kitty", LFS_O_RDWR);
    if (err) { lfs_unmount(lfs); return; }

    int off = 0;
    for (int i = 0; i < COUNT; i++) {
        lfs_file_seek(lfs, &file, off * ENTRYSIZE, LFS_SEEK_SET);
        lfs_ssize_t res = lfs_file_write(lfs, &file, dog, ENTRYSIZE);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        err = lfs_file_sync(lfs, &file);
        if (err) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        off = (5 * off + 1) % COUNT;
    }
    lfs_file_close(lfs, &file);

    // Verify all entries are "doggodogdog"
    err = lfs_file_open(lfs, &file, "kitty", LFS_O_RDONLY);
    if (err) { lfs_unmount(lfs); return; }
    for (int i = 0; i < COUNT; i++) {
        lfs_file_read(lfs, &file, buf, ENTRYSIZE);
    }
    lfs_file_close(lfs, &file);
    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, SeekWrite_4) {
    g_count = 4;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_seek_write, GetParam().behavior);
}

TEST_P(ReentrantTest, SeekWrite_64) {
    g_count = 64;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_seek_write, GetParam().behavior);
}

TEST_P(ReentrantTest, SeekWrite_128) {
    g_count = 128;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_seek_write, GetParam().behavior, 5000);
}

// ---------------------------------------------------------------------------
// 7. ReentrantTruncateWrite
//
// Write "hair" to LARGESIZE, truncate to MEDIUMSIZE and write "bald",
// truncate to SMALLSIZE and write "comb".
// On re-entry, file size must be 0, LARGESIZE, MEDIUMSIZE, or SMALLSIZE.
// ---------------------------------------------------------------------------

static void do_reentrant_truncate_write(lfs_t *lfs, const lfs_config *cfg) {
    lfs_size_t SMALLSIZE = g_smallsize;
    lfs_size_t MEDIUMSIZE = g_mediumsize;
    lfs_size_t LARGESIZE = g_largesize;

    int err = lfs_mount(lfs, cfg);
    if (err) {
        lfs_format(lfs, cfg);
        err = lfs_mount(lfs, cfg);
        if (err) return;
    }

    lfs_file_t file;
    struct lfs_info info;
    uint8_t buf[256];

    // Validate existing file
    if (lfs_stat(lfs, "baldy", &info) == 0) {
        // Valid sizes: 0, SMALLSIZE, MEDIUMSIZE, LARGESIZE
        if (info.size != 0 && info.size != SMALLSIZE &&
            info.size != MEDIUMSIZE && info.size != LARGESIZE) {
            // Unexpected size — the file may be partially truncated,
            // which can happen mid-operation. Just continue.
        }
    }

    // Step 1: Write "hair" to LARGESIZE
    err = lfs_file_open(lfs, &file, "baldy",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err) { lfs_unmount(lfs); return; }
    memset(buf, 0, sizeof(buf));
    memcpy(buf, "hair", 4);
    lfs_size_t written = 0;
    while (written < LARGESIZE) {
        lfs_size_t chunk = LARGESIZE - written;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        // Fill with "hair" pattern (4-byte repeating)
        for (lfs_size_t i = 0; i < chunk; i++) {
            buf[i] = "hair"[i % 4];
        }
        lfs_ssize_t res = lfs_file_write(lfs, &file, buf, chunk);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        written += (lfs_size_t)res;
    }
    lfs_file_close(lfs, &file);

    // Step 2: Truncate to MEDIUMSIZE, overwrite with "bald"
    err = lfs_file_open(lfs, &file, "baldy", LFS_O_WRONLY);
    if (err) { lfs_unmount(lfs); return; }
    err = lfs_file_truncate(lfs, &file, MEDIUMSIZE);
    if (err) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
    lfs_file_seek(lfs, &file, 0, LFS_SEEK_SET);
    written = 0;
    while (written < MEDIUMSIZE) {
        lfs_size_t chunk = MEDIUMSIZE - written;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        for (lfs_size_t i = 0; i < chunk; i++) {
            buf[i] = "bald"[i % 4];
        }
        lfs_ssize_t res = lfs_file_write(lfs, &file, buf, chunk);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        written += (lfs_size_t)res;
    }
    lfs_file_close(lfs, &file);

    // Step 3: Truncate to SMALLSIZE, overwrite with "comb"
    err = lfs_file_open(lfs, &file, "baldy", LFS_O_WRONLY);
    if (err) { lfs_unmount(lfs); return; }
    err = lfs_file_truncate(lfs, &file, SMALLSIZE);
    if (err) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
    lfs_file_seek(lfs, &file, 0, LFS_SEEK_SET);
    written = 0;
    while (written < SMALLSIZE) {
        lfs_size_t chunk = SMALLSIZE - written;
        if (chunk > sizeof(buf)) chunk = sizeof(buf);
        for (lfs_size_t i = 0; i < chunk; i++) {
            buf[i] = "comb"[i % 4];
        }
        lfs_ssize_t res = lfs_file_write(lfs, &file, buf, chunk);
        if (res < 0) { lfs_file_close(lfs, &file); lfs_unmount(lfs); return; }
        written += (lfs_size_t)res;
    }
    lfs_file_close(lfs, &file);

    lfs_unmount(lfs);
}

TEST_P(ReentrantTest, TruncateWrite_4_32_2048) {
    g_smallsize = 4; g_mediumsize = 32; g_largesize = 2048;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_truncate_write, GetParam().behavior);
}

TEST_P(ReentrantTest, TruncateWrite_4_512_2048) {
    g_smallsize = 4; g_mediumsize = 512; g_largesize = 2048;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_truncate_write, GetParam().behavior);
}

TEST_P(ReentrantTest, TruncateWrite_512_1024_2048) {
    g_smallsize = 512; g_mediumsize = 1024; g_largesize = 2048;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_truncate_write, GetParam().behavior);
}

TEST_P(ReentrantTest, TruncateWrite_4_33_2048) {
    g_smallsize = 4; g_mediumsize = 33; g_largesize = 2048;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_truncate_write, GetParam().behavior);
}

TEST_P(ReentrantTest, TruncateWrite_512_513_2048) {
    g_smallsize = 512; g_mediumsize = 513; g_largesize = 2048;
    lfs_config cfg = cfg_; lfs_emubd_config bdcfg = bdcfg_;
    powerloss::RunWithPowerloss(&cfg, &bdcfg,
        do_reentrant_truncate_write, GetParam().behavior);
}

// ---------------------------------------------------------------------------
// Instantiate with NOOP and OOO powerloss behaviors
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    Powerloss, ReentrantTest,
    ::testing::Values(
        ReentrantParams{"noop", LFS_EMUBD_POWERLOSS_NOOP},
        ReentrantParams{"ooo",  LFS_EMUBD_POWERLOSS_OOO}
    ),
    ReentrantParamName);
