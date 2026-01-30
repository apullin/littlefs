/*
 * Power-loss simulation runner for reentrant tests
 *
 * Uses setjmp/longjmp with emubd's power_cycles/powerloss_cb mechanism
 * to simulate power loss at every possible write operation.
 *
 * IMPORTANT: Reentrant test functions must use only POD/C types.
 * No C++ objects with non-trivial destructors may be live on the stack
 * when a power loss occurs (longjmp skips destructors).
 */
#ifndef LFS_POWERLOSS_RUNNER_H
#define LFS_POWERLOSS_RUNNER_H

#include <gtest/gtest.h>
#include <csetjmp>
#include <cstdio>

extern "C" {
#include "lfs.h"
#include "bd/lfs_emubd.h"
}

// Reentrant test function signature.
// Must use only POD/C types — no std::string, std::vector, etc.
// Pattern:
//   int err = lfs_mount(lfs, cfg);
//   if (err) { lfs_format(lfs, cfg); lfs_mount(lfs, cfg); }
//   ... do work ...
//   lfs_unmount(lfs);
typedef void (*ReentrantTestFn)(lfs_t *lfs, const lfs_config *cfg);

namespace powerloss {

// Global powerloss state (single-threaded tests only)
static jmp_buf g_env;
static bool g_active = false;

static void powerloss_cb(void *) {
    if (g_active) {
        g_active = false;
        longjmp(g_env, 1);
    }
}

// Run a reentrant test with linear powerloss simulation.
// Calls test_fn repeatedly with power_cycles = 1, 2, 3, ...
// until the test completes without triggering a power loss.
//
// The emubd block device is created once and persists across iterations,
// retaining partially-written state from each simulated power loss.
//
// cfg and bdcfg must be fully initialized before calling.
// cfg->context will be set to the internal emubd instance.
inline void RunWithPowerloss(
    lfs_config *cfg,
    lfs_emubd_config *bdcfg,
    ReentrantTestFn test_fn,
    lfs_emubd_powerloss_behavior_t behavior = LFS_EMUBD_POWERLOSS_NOOP,
    int max_cycles = 1000)
{
    // Configure powerloss callback
    bdcfg->powerloss_cb = powerloss_cb;
    bdcfg->powerloss_behavior = behavior;
    bdcfg->power_cycles = 0;  // disabled initially

    // Create block device
    lfs_emubd_t bd;
    memset(&bd, 0, sizeof(bd));
    cfg->context = &bd;
    int err = lfs_emubd_create(cfg, bdcfg);
    ASSERT_EQ(err, 0) << "Failed to create emubd for powerloss test";

    // Format the filesystem initially
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, cfg), 0) << "Initial format failed";

    for (int cycle = 1; cycle <= max_cycles; cycle++) {
        // Set power_cycles countdown
        lfs_emubd_setpowercycles(cfg, cycle);

        g_active = true;
        if (setjmp(g_env) != 0) {
            // Returned from longjmp — power loss occurred.
            // The emubd retains its partially-written state.
            // Continue to next cycle.
            continue;
        }

        // Run the reentrant test body
        test_fn(&lfs, cfg);

        // If we reach here, the test completed without power loss
        g_active = false;
        lfs_emubd_destroy(cfg);
        return;
    }

    // Cleanup on max_cycles exceeded
    g_active = false;
    lfs_emubd_destroy(cfg);
    FAIL() << "Reentrant test did not complete within " << max_cycles
           << " power-loss cycles";
}

} // namespace powerloss

// Fixture base for reentrant tests parameterized on powerloss behavior
struct PowerlossBehaviorParam {
    const char *name;
    lfs_emubd_powerloss_behavior_t behavior;
};

inline std::string PowerlossBehaviorName(
    const ::testing::TestParamInfo<PowerlossBehaviorParam> &info) {
    return info.param.name;
}

#endif // LFS_POWERLOSS_RUNNER_H
