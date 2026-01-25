/*
 * Path handling tests - various path formats and edge cases
 */
#include "lfs_test_fixture.h"
#include "lfs_test_macros.h"
#include <cstring>
#include <cstdio>

class PathsTest : public LfsParametricTest {};

// Simple paths test with directories
TEST_P(PathsTest, SimpleDir) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create directory structure
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/drip"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/coldbrew"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/turkish"), 0);

    // Stat paths
    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);
    ASSERT_EQ(lfs_stat(&lfs, "coffee/coldbrew", &info), 0);
    ASSERT_STREQ(info.name, "coldbrew");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    // Dir open paths - should work on directories
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "coffee/drip"), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // File open on directory should fail
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "coffee/drip", LFS_O_RDONLY), LFS_ERR_ISDIR);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Simple paths test with files
TEST_P(PathsTest, SimpleFile) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create directory and files
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee"), 0);
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "coffee/drip",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "coffee/coldbrew",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Stat paths
    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(info.type, LFS_TYPE_REG);

    // File open paths - should work on files
    ASSERT_EQ(lfs_file_open(&lfs, &file, "coffee/drip", LFS_O_RDONLY), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Dir open on file should fail
    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "coffee/drip"), LFS_ERR_NOTDIR);

    // O_EXCL on existing should fail
    ASSERT_EQ(lfs_file_open(&lfs, &file, "coffee/drip",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL), LFS_ERR_EXIST);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Absolute paths (leading slash)
TEST_P(PathsTest, Absolute) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "/coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "/coffee/drip"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "/coffee/drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/coffee"), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Redundant slashes
TEST_P(PathsTest, RedundantSlashes) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Multiple leading slashes
    ASSERT_EQ(lfs_mkdir(&lfs, "///coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee//drip"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee///coldbrew"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(lfs_stat(&lfs, "coffee//coldbrew", &info), 0);
    ASSERT_STREQ(info.name, "coldbrew");
    ASSERT_EQ(lfs_stat(&lfs, "///coffee///drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Trailing slashes
TEST_P(PathsTest, TrailingSlashes) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/drip/"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "coffee/", &info), 0);
    ASSERT_STREQ(info.name, "coffee");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip///", &info), 0);
    ASSERT_STREQ(info.name, "drip");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Dot paths (current directory)
TEST_P(PathsTest, Dots) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/drip"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "./coffee", &info), 0);
    ASSERT_STREQ(info.name, "coffee");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/./drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/.", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(lfs_stat(&lfs, "././coffee/./drip/.", &info), 0);
    ASSERT_STREQ(info.name, "drip");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Dotdot paths (parent directory)
TEST_P(PathsTest, DotDots) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/drip"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/..", &info), 0);
    ASSERT_STREQ(info.name, "coffee");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/../drip", &info), 0);
    ASSERT_STREQ(info.name, "drip");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/../drip/../..", &info), 0);
    ASSERT_STREQ(info.name, "/");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Root dotdots via subdirectory
TEST_P(PathsTest, RootDotDots) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "coffee/drip"), 0);

    struct lfs_info info;
    // Dotdots from subdirectory back to parent
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/..", &info), 0);
    ASSERT_STREQ(info.name, "coffee");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/../..", &info), 0);
    ASSERT_STREQ(info.name, "/");
    ASSERT_EQ(lfs_stat(&lfs, "coffee/drip/../../coffee", &info), 0);
    ASSERT_STREQ(info.name, "coffee");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Path not found (NOENT)
TEST_P(PathsTest, Noent) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "nonexistent", &info), LFS_ERR_NOENT);
    ASSERT_EQ(lfs_stat(&lfs, "a/b/c", &info), LFS_ERR_NOENT);

    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "nonexistent"), LFS_ERR_NOENT);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "nonexistent", LFS_O_RDONLY), LFS_ERR_NOENT);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Parent not found
TEST_P(PathsTest, NoentParent) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Try to create in non-existent parent
    ASSERT_EQ(lfs_mkdir(&lfs, "nonexistent/child"), LFS_ERR_NOENT);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "nonexistent/file",
            LFS_O_WRONLY | LFS_O_CREAT), LFS_ERR_NOENT);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Parent is not a directory
TEST_P(PathsTest, NotdirParent) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create a file
    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "afile",
            LFS_O_WRONLY | LFS_O_CREAT), 0);
    ASSERT_EQ(lfs_file_close(&lfs, &file), 0);

    // Try to use file as directory
    ASSERT_EQ(lfs_mkdir(&lfs, "afile/child"), LFS_ERR_NOTDIR);
    ASSERT_EQ(lfs_file_open(&lfs, &file, "afile/file",
            LFS_O_WRONLY | LFS_O_CREAT), LFS_ERR_NOTDIR);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "afile/child", &info), LFS_ERR_NOTDIR);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Empty path
TEST_P(PathsTest, Empty) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "", &info), LFS_ERR_INVAL);

    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, ""), LFS_ERR_INVAL);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, "", LFS_O_RDONLY), LFS_ERR_INVAL);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Root paths
TEST_P(PathsTest, Root) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "/", &info), 0);
    ASSERT_STREQ(info.name, "/");
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    lfs_dir_t dir;
    ASSERT_EQ(lfs_dir_open(&lfs, &dir, "/"), 0);
    ASSERT_EQ(lfs_dir_close(&lfs, &dir), 0);

    // Can't remove or rename root
    ASSERT_EQ(lfs_remove(&lfs, "/"), LFS_ERR_INVAL);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Root aliases (/, /., /./.)
TEST_P(PathsTest, RootAliases) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "/", &info), 0);
    ASSERT_STREQ(info.name, "/");
    ASSERT_EQ(lfs_stat(&lfs, "/.", &info), 0);
    ASSERT_STREQ(info.name, "/");
    ASSERT_EQ(lfs_stat(&lfs, "/./.", &info), 0);
    ASSERT_STREQ(info.name, "/");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Name too long
TEST_P(PathsTest, NameTooLong) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create a name that's too long (LFS_NAME_MAX + 1)
    char longname[LFS_NAME_MAX + 32];
    memset(longname, 'a', LFS_NAME_MAX + 1);
    longname[LFS_NAME_MAX + 1] = '\0';

    ASSERT_EQ(lfs_mkdir(&lfs, longname), LFS_ERR_NAMETOOLONG);

    lfs_file_t file;
    ASSERT_EQ(lfs_file_open(&lfs, &file, longname,
            LFS_O_WRONLY | LFS_O_CREAT), LFS_ERR_NAMETOOLONG);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Name just long enough (exactly LFS_NAME_MAX)
TEST_P(PathsTest, NameJustLongEnough) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create a name that's exactly LFS_NAME_MAX
    char maxname[LFS_NAME_MAX + 1];
    memset(maxname, 'b', LFS_NAME_MAX);
    maxname[LFS_NAME_MAX] = '\0';

    ASSERT_EQ(lfs_mkdir(&lfs, maxname), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, maxname, &info), 0);
    ASSERT_STREQ(info.name, maxname);
    ASSERT_EQ(info.type, LFS_TYPE_DIR);

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// UTF-8 paths
TEST_P(PathsTest, Utf8) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    // Create directories with UTF-8 names
    ASSERT_EQ(lfs_mkdir(&lfs, "кафе"), 0);  // Russian
    ASSERT_EQ(lfs_mkdir(&lfs, "カフェ"), 0);  // Japanese
    ASSERT_EQ(lfs_mkdir(&lfs, "咖啡"), 0);   // Chinese

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "кафе", &info), 0);
    ASSERT_STREQ(info.name, "кафе");
    ASSERT_EQ(lfs_stat(&lfs, "カフェ", &info), 0);
    ASSERT_STREQ(info.name, "カフェ");
    ASSERT_EQ(lfs_stat(&lfs, "咖啡", &info), 0);
    ASSERT_STREQ(info.name, "咖啡");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

// Paths with spaces
TEST_P(PathsTest, Spaces) {
    lfs_t lfs;
    ASSERT_EQ(lfs_format(&lfs, &cfg_), 0);
    ASSERT_EQ(lfs_mount(&lfs, &cfg_), 0);

    ASSERT_EQ(lfs_mkdir(&lfs, "my coffee"), 0);
    ASSERT_EQ(lfs_mkdir(&lfs, "my coffee/cold brew"), 0);

    struct lfs_info info;
    ASSERT_EQ(lfs_stat(&lfs, "my coffee", &info), 0);
    ASSERT_STREQ(info.name, "my coffee");
    ASSERT_EQ(lfs_stat(&lfs, "my coffee/cold brew", &info), 0);
    ASSERT_STREQ(info.name, "cold brew");

    // All spaces
    ASSERT_EQ(lfs_mkdir(&lfs, "   "), 0);
    ASSERT_EQ(lfs_stat(&lfs, "   ", &info), 0);
    ASSERT_STREQ(info.name, "   ");

    ASSERT_EQ(lfs_unmount(&lfs), 0);
}

INSTANTIATE_TEST_SUITE_P(
    Geometries, PathsTest,
    ::testing::ValuesIn(AllGeometries()),
    GeometryNameGenerator{});
