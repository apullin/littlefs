/*
 * Compatibility layer for littlefs tests
 * Maps between gtest (host) and ztest (Zephyr) frameworks
 */
#ifndef LFS_TEST_COMPAT_H
#define LFS_TEST_COMPAT_H

#include "lfs.h"

#if defined(LFS_TEST_GTEST)
    // gtest - assertions are C++ macros, defined in gtest wrapper
    // These are placeholders that get redefined in the C++ context

#elif defined(LFS_TEST_ZTEST)
    #include <zephyr/ztest.h>

    #define LFS_TEST_ASSERT_OK(expr) \
        zassert_equal((expr), 0, "Failed: " #expr)

    #define LFS_TEST_ASSERT_EQ(actual, expected) \
        zassert_equal((actual), (expected), \
            "Expected " #actual " == " #expected)

    #define LFS_TEST_ASSERT_NE(actual, expected) \
        zassert_not_equal((actual), (expected), \
            "Expected " #actual " != " #expected)

    #define LFS_TEST_ASSERT_TRUE(expr) \
        zassert_true((expr), "Failed: " #expr)

    #define LFS_TEST_ASSERT_FALSE(expr) \
        zassert_false((expr), "Failed: " #expr)

    #define LFS_TEST_FAIL(msg) \
        zassert_true(false, msg)

#else
    #error "Must define LFS_TEST_GTEST or LFS_TEST_ZTEST"
#endif

#endif /* LFS_TEST_COMPAT_H */
