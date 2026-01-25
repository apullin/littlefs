#ifndef LFS_TEST_MACROS_H
#define LFS_TEST_MACROS_H

#include <gtest/gtest.h>
#include <cstdint>

// Assertion macros to replace the TOML "=>" syntax
// Example: lfs_mount(&lfs, cfg) => 0  becomes  LFS_ASSERT_OK(lfs_mount(&lfs_, &cfg_))

#define LFS_ASSERT_OK(expr) \
    do { \
        int _result = (expr); \
        ASSERT_EQ(_result, 0) << "Expression failed: " #expr; \
    } while(0)

#define LFS_EXPECT_OK(expr) \
    do { \
        int _result = (expr); \
        EXPECT_EQ(_result, 0) << "Expression failed: " #expr; \
    } while(0)

#define LFS_ASSERT_EQ(expr, expected) \
    do { \
        auto _result = (expr); \
        auto _expected = (expected); \
        ASSERT_EQ(_result, _expected) << "Expression: " #expr; \
    } while(0)

#define LFS_EXPECT_EQ(expr, expected) \
    do { \
        auto _result = (expr); \
        auto _expected = (expected); \
        EXPECT_EQ(_result, _expected) << "Expression: " #expr; \
    } while(0)

#define LFS_ASSERT_NE(expr, expected) \
    do { \
        auto _result = (expr); \
        auto _expected = (expected); \
        ASSERT_NE(_result, _expected) << "Expression: " #expr; \
    } while(0)

#define LFS_EXPECT_NE(expr, expected) \
    do { \
        auto _result = (expr); \
        auto _expected = (expected); \
        EXPECT_NE(_result, _expected) << "Expression: " #expr; \
    } while(0)

// For checking LFS error codes
#define LFS_ASSERT_ERR(expr, err) LFS_ASSERT_EQ(expr, err)
#define LFS_EXPECT_ERR(expr, err) LFS_EXPECT_EQ(expr, err)

// PRNG for generating test data - matches the test framework's TEST_PRNG
inline uint32_t lfs_test_prng(uint32_t *state) {
    *state = *state * 1103515245 + 12345;
    return *state;
}

#define TEST_PRNG(state) lfs_test_prng(state)

// Min/max utilities
#ifndef lfs_min
#define lfs_min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef lfs_max
#define lfs_max(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // LFS_TEST_MACROS_H
