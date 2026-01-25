#ifndef LFS_TEST_MACROS_H
#define LFS_TEST_MACROS_H

#include <gtest/gtest.h>

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

#endif // LFS_TEST_MACROS_H
