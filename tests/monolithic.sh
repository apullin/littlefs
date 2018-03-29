
OUTPUTFILE="pre_gen_tests.txt"

rm -rf OUTPUTFILE

printf() {
cat >> $OUTPUTFILE << TEST
    printf("$1\n");
TEST
}


#############################
####### test_format.sh ######
#############################
printf "=== Formatting tests ===\n"
# rm -rf blocks

printf "--- Basic formatting ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

# TODO: this needs an equivalent
# printf "--- Invalid superblocks ---\n"
# ln -f -s /dev/zero blocks/0
# ln -f -s /dev/zero blocks/1
# cat >> $OUTPUTFILE << TEST
#     lfs_format(&lfs, &cfg) => LFS_ERR_CORRUPT;
# TEST
# rm blocks/0 blocks/1

printf "--- Basic mounting ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Invalid mount ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST
# TODO: this needs an equivalent
# rm blocks/0 blocks/1
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => LFS_ERR_CORRUPT;
# TEST

printf "--- Valid corrupt mount ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST
# TODO: this needs an equivalent
# rm blocks/0
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => 0;
#     lfs_unmount(&lfs) => 0;
# TEST

printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
####### test_dirs.sh ########
#############################

LARGESIZE=128

printf "=== Directory tests ==="
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

printf "--- Root directory ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Directory creation ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "potato") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- File creation ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "burito", LFS_O_CREAT | LFS_O_WRONLY) => 0;
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Directory iteration ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "potato") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "burito") => 0;
    info.type => LFS_TYPE_REG;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Directory failures ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "potato") => LFS_ERR_EXIST;
    lfs_dir_open(&lfs, &dir[0], "tomato") => LFS_ERR_NOENT;
    lfs_dir_open(&lfs, &dir[0], "burito") => LFS_ERR_NOTDIR;
    lfs_file_open(&lfs, &file[0], "tomato", LFS_O_RDONLY) => LFS_ERR_NOENT;
    lfs_file_open(&lfs, &file[0], "potato", LFS_O_RDONLY) => LFS_ERR_ISDIR;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Nested directories ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "potato/baked") => 0;
    lfs_mkdir(&lfs, "potato/sweet") => 0;
    lfs_mkdir(&lfs, "potato/fried") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "potato") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "baked") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "sweet") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "fried") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Multi-block directory ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "cactus") => 0;
    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "cactus/test%d", i);
        lfs_mkdir(&lfs, (char*)buffer) => 0;
    }
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "cactus") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "test%d", i);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;
        info.type => LFS_TYPE_DIR;
    }
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Directory remove ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "potato") => LFS_ERR_NOTEMPTY;
    lfs_remove(&lfs, "potato/sweet") => 0;
    lfs_remove(&lfs, "potato/baked") => 0;
    lfs_remove(&lfs, "potato/fried") => 0;

    lfs_dir_open(&lfs, &dir[0], "potato") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;

    lfs_remove(&lfs, "potato") => 0;

    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "burito") => 0;
    info.type => LFS_TYPE_REG;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "cactus") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "burito") => 0;
    info.type => LFS_TYPE_REG;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "cactus") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Directory rename ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "coldpotato") => 0;
    lfs_mkdir(&lfs, "coldpotato/baked") => 0;
    lfs_mkdir(&lfs, "coldpotato/sweet") => 0;
    lfs_mkdir(&lfs, "coldpotato/fried") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "coldpotato", "hotpotato") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "hotpotato") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "baked") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "sweet") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "fried") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "warmpotato") => 0;
    lfs_mkdir(&lfs, "warmpotato/mushy") => 0;
    lfs_rename(&lfs, "hotpotato", "warmpotato") => LFS_ERR_NOTEMPTY;

    lfs_remove(&lfs, "warmpotato/mushy") => 0;
    lfs_rename(&lfs, "hotpotato", "warmpotato") => 0;

    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "warmpotato") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "baked") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "sweet") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "fried") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "coldpotato") => 0;
    lfs_rename(&lfs, "warmpotato/baked", "coldpotato/baked") => 0;
    lfs_rename(&lfs, "warmpotato/sweet", "coldpotato/sweet") => 0;
    lfs_rename(&lfs, "warmpotato/fried", "coldpotato/fried") => 0;
    lfs_remove(&lfs, "coldpotato") => LFS_ERR_NOTEMPTY;
    lfs_remove(&lfs, "warmpotato") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "coldpotato") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "baked") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "sweet") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "fried") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Recursive remove ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "coldpotato") => LFS_ERR_NOTEMPTY;

    lfs_dir_open(&lfs, &dir[0], "coldpotato") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;

    while (true) {
        int err = lfs_dir_read(&lfs, &dir[0], &info);
        err >= 0 => 1;
        if (err == 0) {
            break;
        }

        strcpy((char*)buffer, "coldpotato/");
        strcat((char*)buffer, info.name);
        lfs_remove(&lfs, (char*)buffer) => 0;
    }

    lfs_remove(&lfs, "coldpotato") => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "burito") => 0;
    info.type => LFS_TYPE_REG;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "cactus") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Multi-block remove ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "cactus") => LFS_ERR_NOTEMPTY;

    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "cactus/test%d", i);
        lfs_remove(&lfs, (char*)buffer) => 0;
    }

    lfs_remove(&lfs, "cactus") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "burito") => 0;
    info.type => LFS_TYPE_REG;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Multi-block directory with files ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "prickly-pear") => 0;
    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "prickly-pear/test%d", i);
        lfs_file_open(&lfs, &file[0], (char*)buffer,
                LFS_O_WRONLY | LFS_O_CREAT) => 0;
        size = 6;
        memcpy(wbuffer, "Hello", size);
        lfs_file_write(&lfs, &file[0], wbuffer, size) => size;
        lfs_file_close(&lfs, &file[0]) => 0;
    }
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "prickly-pear") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "test%d", i);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;
        info.type => LFS_TYPE_REG;
        info.size => 6;
    }
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Multi-block remove with files ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "prickly-pear") => LFS_ERR_NOTEMPTY;

    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "prickly-pear/test%d", i);
        lfs_remove(&lfs, (char*)buffer) => 0;
    }

    lfs_remove(&lfs, "prickly-pear") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "burito") => 0;
    info.type => LFS_TYPE_REG;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Results ---"
printf "IMPLEMENT RESULTS FUNCTION HERE"


#############################
####### test_files.sh #######
#############################

SMALLSIZE=32
MEDIUMSIZE=8192
LARGESIZE=262144

printf "=== File tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

printf "--- Simple file test ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello", LFS_O_WRONLY | LFS_O_CREAT) => 0;
    size = strlen("Hello World!\n");
    memcpy(wbuffer, "Hello World!\n", size);
    lfs_file_write(&lfs, &file[0], wbuffer, size) => size;
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_file_open(&lfs, &file[0], "hello", LFS_O_RDONLY) => 0;
    size = strlen("Hello World!\n");
    lfs_file_read(&lfs, &file[0], rbuffer, size) => size;
    memcmp(rbuffer, wbuffer, size) => 0;
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

w_test() {
cat >> $OUTPUTFILE << TEST
    {
        lfs_size_t size = $1;
        lfs_size_t chunk = 31;
        srand(0);
        lfs_mount(&lfs, &cfg) => 0;
        lfs_file_open(&lfs, &file[0], "$2",
            ${3:-LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC}) => 0;
        for (lfs_size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            for (lfs_size_t b = 0; b < chunk; b++) {
                buffer[b] = rand() & 0xff;
            }
            lfs_file_write(&lfs, &file[0], buffer, chunk) => chunk;
        }
        lfs_file_close(&lfs, &file[0]) => 0;
        lfs_unmount(&lfs) => 0;
    }
TEST
}

r_test() {
cat >> $OUTPUTFILE << TEST
    {
        lfs_size_t size = $1;
        lfs_size_t chunk = 29;
        srand(0);
        lfs_mount(&lfs, &cfg) => 0;
        lfs_stat(&lfs, "$2", &info) => 0;
        info.type => LFS_TYPE_REG;
        info.size => size;
        lfs_file_open(&lfs, &file[0], "$2", ${3:-LFS_O_RDONLY}) => 0;
        for (lfs_size_t i = 0; i < size; i += chunk) {
            chunk = (chunk < size - i) ? chunk : size - i;
            lfs_file_read(&lfs, &file[0], buffer, chunk) => chunk;
            for (lfs_size_t b = 0; b < chunk && i+b < size; b++) {
                buffer[b] => rand() & 0xff;
            }
        }
        lfs_file_close(&lfs, &file[0]) => 0;
        lfs_unmount(&lfs) => 0;
    }
TEST
}

printf "--- Small file test ---\n"
w_test $SMALLSIZE smallavacado
r_test $SMALLSIZE smallavacado

printf "--- Medium file test ---\n"
w_test $MEDIUMSIZE mediumavacado
r_test $MEDIUMSIZE mediumavacado

printf "--- Large file test ---\n"
w_test $LARGESIZE largeavacado
r_test $LARGESIZE largeavacado

printf "--- Zero file test ---\n"
w_test 0 noavacado
r_test 0 noavacado

printf "--- Truncate small test ---\n"
w_test $SMALLSIZE mediumavacado
r_test $SMALLSIZE mediumavacado
w_test $MEDIUMSIZE mediumavacado
r_test $MEDIUMSIZE mediumavacado

printf "--- Truncate zero test ---\n"
w_test $SMALLSIZE noavacado
r_test $SMALLSIZE noavacado
w_test 0 noavacado
r_test 0 noavacado

printf "--- Non-overlap check ---\n"
r_test $SMALLSIZE smallavacado
r_test $MEDIUMSIZE mediumavacado
r_test $LARGESIZE largeavacado
r_test 0 noavacado

printf "--- Dir check ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "hello") => 0;
    info.type => LFS_TYPE_REG;
    info.size => strlen("Hello World!\n");
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "smallavacado") => 0;
    info.type => LFS_TYPE_REG;
    info.size => $SMALLSIZE;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "mediumavacado") => 0;
    info.type => LFS_TYPE_REG;
    info.size => $MEDIUMSIZE;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "largeavacado") => 0;
    info.type => LFS_TYPE_REG;
    info.size => $LARGESIZE;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "noavacado") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Many file test ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    // Create 300 files of 6 bytes
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "directory") => 0;
    for (unsigned i = 0; i < 300; i++) {
        snprintf((char*)buffer, sizeof(buffer), "file_%03d", i);
        lfs_file_open(&lfs, &file[0], (char*)buffer, LFS_O_WRONLY | LFS_O_CREAT) => 0;
        size = 6;
        memcpy(wbuffer, "Hello", size);
        lfs_file_write(&lfs, &file[0], wbuffer, size) => size;
        lfs_file_close(&lfs, &file[0]) => 0;
    }
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
####### test_seek.sh ########
#############################
SMALLSIZE=4
MEDIUMSIZE=128
LARGESIZE=132

printf "=== Seek tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "hello") => 0;
    for (int i = 0; i < $LARGESIZE; i++) {
        sprintf((char*)buffer, "hello/kitty%d", i);
        lfs_file_open(&lfs, &file[0], (char*)buffer,
                LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) => 0;

        size = strlen("kittycatcat");
        memcpy(buffer, "kittycatcat", size);
        for (int j = 0; j < $LARGESIZE; j++) {
            lfs_file_write(&lfs, &file[0], buffer, size);
        }

        lfs_file_close(&lfs, &file[0]) => 0;
    }
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Simple dir seek ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "hello") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;

    {
        lfs_soff_t pos;
        int i;
        for (i = 0; i < $SMALLSIZE; i++) {
            sprintf((char*)buffer, "kitty%d", i);
            lfs_dir_read(&lfs, &dir[0], &info) => 1;
            strcmp(info.name, (char*)buffer) => 0;
            pos = lfs_dir_tell(&lfs, &dir[0]);
        }
        pos >= 0 => 1;

        lfs_dir_seek(&lfs, &dir[0], pos) => 0;
        sprintf((char*)buffer, "kitty%d", i);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;

        lfs_dir_rewind(&lfs, &dir[0]) => 0;
        sprintf((char*)buffer, "kitty%d", 0);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, ".") => 0;
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, "..") => 0;
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;

        lfs_dir_seek(&lfs, &dir[0], pos) => 0;
        sprintf((char*)buffer, "kitty%d", i);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;
    }

    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Large dir seek ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "hello") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;

    {
        lfs_soff_t pos;
        int i;
        for (i = 0; i < $MEDIUMSIZE; i++) {
            sprintf((char*)buffer, "kitty%d", i);
            lfs_dir_read(&lfs, &dir[0], &info) => 1;
            strcmp(info.name, (char*)buffer) => 0;
            pos = lfs_dir_tell(&lfs, &dir[0]);
        }
        pos >= 0 => 1;

        lfs_dir_seek(&lfs, &dir[0], pos) => 0;
        sprintf((char*)buffer, "kitty%d", i);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;

        lfs_dir_rewind(&lfs, &dir[0]) => 0;
        sprintf((char*)buffer, "kitty%d", 0);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, ".") => 0;
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, "..") => 0;
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;

        lfs_dir_seek(&lfs, &dir[0], pos) => 0;
        sprintf((char*)buffer, "kitty%d", i);
        lfs_dir_read(&lfs, &dir[0], &info) => 1;
        strcmp(info.name, (char*)buffer) => 0;
    }

    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Simple file seek ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello/kitty42", LFS_O_RDONLY) => 0;

    {
        lfs_soff_t pos;
        size = strlen("kittycatcat");
        for (int i = 0; i < $SMALLSIZE; i++) {
            lfs_file_read(&lfs, &file[0], buffer, size) => size;
            memcmp(buffer, "kittycatcat", size) => 0;
            pos = lfs_file_tell(&lfs, &file[0]);
        }
        pos >= 0 => 1;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_rewind(&lfs, &file[0]) => 0;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_CUR) => size;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], size, LFS_SEEK_CUR) => 3*size;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], -size, LFS_SEEK_CUR) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], -size, LFS_SEEK_END) >= 0 => 1;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_size_t size = lfs_file_size(&lfs, &file[0]);
        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_CUR) => size;
    }

    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Large file seek ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello/kitty42", LFS_O_RDONLY) => 0;

    {
        lfs_soff_t pos;
        size = strlen("kittycatcat");
        for (int i = 0; i < $MEDIUMSIZE; i++) {
            lfs_file_read(&lfs, &file[0], buffer, size) => size;
            memcmp(buffer, "kittycatcat", size) => 0;
            pos = lfs_file_tell(&lfs, &file[0]);
        }
        pos >= 0 => 1;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_rewind(&lfs, &file[0]) => 0;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_CUR) => size;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], size, LFS_SEEK_CUR) => 3*size;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], -size, LFS_SEEK_CUR) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], -size, LFS_SEEK_END) >= 0 => 1;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_size_t size = lfs_file_size(&lfs, &file[0]);
        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_CUR) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Simple file seek and write ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello/kitty42", LFS_O_RDWR) => 0;

    {
        lfs_soff_t pos;
        size = strlen("kittycatcat");
        for (int i = 0; i < $SMALLSIZE; i++) {
            lfs_file_read(&lfs, &file[0], buffer, size) => size;
            memcmp(buffer, "kittycatcat", size) => 0;
            pos = lfs_file_tell(&lfs, &file[0]);
        }
        pos >= 0 => 1;

        memcpy(buffer, "doggodogdog", size);
        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_write(&lfs, &file[0], buffer, size) => size;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "doggodogdog", size) => 0;

        lfs_file_rewind(&lfs, &file[0]) => 0;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "doggodogdog", size) => 0;

        lfs_file_seek(&lfs, &file[0], -size, LFS_SEEK_END) >= 0 => 1;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_size_t size = lfs_file_size(&lfs, &file[0]);
        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_CUR) => size;
    }

    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Large file seek and write ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello/kitty42", LFS_O_RDWR) => 0;

    {
        lfs_soff_t pos;
        size = strlen("kittycatcat");
        for (int i = 0; i < $MEDIUMSIZE; i++) {
            lfs_file_read(&lfs, &file[0], buffer, size) => size;
            if (i != $SMALLSIZE) {
                memcmp(buffer, "kittycatcat", size) => 0;
            }
            pos = lfs_file_tell(&lfs, &file[0]);
        }
        pos >= 0 => 1;

        memcpy(buffer, "doggodogdog", size);
        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_write(&lfs, &file[0], buffer, size) => size;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "doggodogdog", size) => 0;

        lfs_file_rewind(&lfs, &file[0]) => 0;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_seek(&lfs, &file[0], pos, LFS_SEEK_SET) => pos;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "doggodogdog", size) => 0;

        lfs_file_seek(&lfs, &file[0], -size, LFS_SEEK_END) >= 0 => 1;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_size_t size = lfs_file_size(&lfs, &file[0]);
        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_CUR) => size;
    }

    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Boundary seek and write ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello/kitty42", LFS_O_RDWR) => 0;

    size = strlen("hedgehoghog");
    const lfs_soff_t offsets[] = {512, 1020, 513, 1021, 511, 1019};

    for (int i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        lfs_soff_t off = offsets[i];
        memcpy(buffer, "hedgehoghog", size);
        lfs_file_seek(&lfs, &file[0], off, LFS_SEEK_SET) => off;
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
        lfs_file_seek(&lfs, &file[0], off, LFS_SEEK_SET) => off;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "hedgehoghog", size) => 0;

        lfs_file_seek(&lfs, &file[0], 0, LFS_SEEK_SET) => 0;
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "kittycatcat", size) => 0;

        lfs_file_sync(&lfs, &file[0]) => 0;
    }

    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Out-of-bounds seek ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "hello/kitty42", LFS_O_RDWR) => 0;

    size = strlen("kittycatcat");
    lfs_file_size(&lfs, &file[0]) => $LARGESIZE*size;
    lfs_file_seek(&lfs, &file[0], ($LARGESIZE+$SMALLSIZE)*size,
            LFS_SEEK_SET) => ($LARGESIZE+$SMALLSIZE)*size;
    lfs_file_read(&lfs, &file[0], buffer, size) => 0;

    memcpy(buffer, "porcupineee", size);
    lfs_file_write(&lfs, &file[0], buffer, size) => size;

    lfs_file_seek(&lfs, &file[0], ($LARGESIZE+$SMALLSIZE)*size,
            LFS_SEEK_SET) => ($LARGESIZE+$SMALLSIZE)*size;
    lfs_file_read(&lfs, &file[0], buffer, size) => size;
    memcmp(buffer, "porcupineee", size) => 0;

    lfs_file_seek(&lfs, &file[0], $LARGESIZE*size,
            LFS_SEEK_SET) => $LARGESIZE*size;
    lfs_file_read(&lfs, &file[0], buffer, size) => size;
    memcmp(buffer, "\0\0\0\0\0\0\0\0\0\0\0", size) => 0;

    lfs_file_seek(&lfs, &file[0], -(($LARGESIZE+$SMALLSIZE)*size),
            LFS_SEEK_CUR) => LFS_ERR_INVAL;
    lfs_file_tell(&lfs, &file[0]) => ($LARGESIZE+1)*size;

    lfs_file_seek(&lfs, &file[0], -(($LARGESIZE+2*$SMALLSIZE)*size),
            LFS_SEEK_END) => LFS_ERR_INVAL;
    lfs_file_tell(&lfs, &file[0]) => ($LARGESIZE+1)*size;

    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
###### test_truncate.sh #####
#############################
SMALLSIZE=32
MEDIUMSIZE=2048
LARGESIZE=8192

printf "=== Truncate tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

truncate_test() {
STARTSIZES="$1"
STARTSEEKS="$2"
HOTSIZES="$3"
COLDSIZES="$4"
cat >> $OUTPUTFILE << TEST
    {
        static const lfs_off_t startsizes[] = {$STARTSIZES};
        static const lfs_off_t startseeks[] = {$STARTSEEKS};
        static const lfs_off_t hotsizes[]   = {$HOTSIZES};

        lfs_mount(&lfs, &cfg) => 0;

        for (int i = 0; i < sizeof(startsizes)/sizeof(startsizes[0]); i++) {
            sprintf((char*)buffer, "hairyhead%d", i);
            lfs_file_open(&lfs, &file[0], (const char*)buffer,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) => 0;

            strcpy((char*)buffer, "hair");
            size = strlen((char*)buffer);
            for (int j = 0; j < startsizes[i]; j += size) {
                lfs_file_write(&lfs, &file[0], buffer, size) => size;
            }
            lfs_file_size(&lfs, &file[0]) => startsizes[i];

            if (startseeks[i] != startsizes[i]) {
                lfs_file_seek(&lfs, &file[0],
                        startseeks[i], LFS_SEEK_SET) => startseeks[i];
            }

            lfs_file_truncate(&lfs, &file[0], hotsizes[i]) => 0;
            lfs_file_size(&lfs, &file[0]) => hotsizes[i];

            lfs_file_close(&lfs, &file[0]) => 0;
        }
    }

    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    {
        static const lfs_off_t startsizes[] = {$STARTSIZES};
        static const lfs_off_t hotsizes[]   = {$HOTSIZES};
        static const lfs_off_t coldsizes[]  = {$COLDSIZES};

        lfs_mount(&lfs, &cfg) => 0;

        for (int i = 0; i < sizeof(startsizes)/sizeof(startsizes[0]); i++) {
            sprintf((char*)buffer, "hairyhead%d", i);
            lfs_file_open(&lfs, &file[0], (const char*)buffer, LFS_O_RDWR) => 0;
            lfs_file_size(&lfs, &file[0]) => hotsizes[i];

            size = strlen("hair");
            int j = 0;
            for (; j < startsizes[i] && j < hotsizes[i]; j += size) {
                lfs_file_read(&lfs, &file[0], buffer, size) => size;
                memcmp(buffer, "hair", size) => 0;
            }

            for (; j < hotsizes[i]; j += size) {
                lfs_file_read(&lfs, &file[0], buffer, size) => size;
                memcmp(buffer, "\0\0\0\0", size) => 0;
            }

            lfs_file_truncate(&lfs, &file[0], coldsizes[i]) => 0;
            lfs_file_size(&lfs, &file[0]) => coldsizes[i];

            lfs_file_close(&lfs, &file[0]) => 0;
        }
    }
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    {
        static const lfs_off_t startsizes[] = {$STARTSIZES};
        static const lfs_off_t hotsizes[]   = {$HOTSIZES};
        static const lfs_off_t coldsizes[]  = {$COLDSIZES};

        lfs_mount(&lfs, &cfg) => 0;

        for (int i = 0; i < sizeof(startsizes)/sizeof(startsizes[0]); i++) {
            sprintf((char*)buffer, "hairyhead%d", i);
            lfs_file_open(&lfs, &file[0], (const char*)buffer, LFS_O_RDONLY) => 0;
            lfs_file_size(&lfs, &file[0]) => coldsizes[i];

            size = strlen("hair");
            int j = 0;
            for (; j < startsizes[i] && j < hotsizes[i] && j < coldsizes[i];
                    j += size) {
                lfs_file_read(&lfs, &file[0], buffer, size) => size;
                memcmp(buffer, "hair", size) => 0;
            }

            for (; j < coldsizes[i]; j += size) {
                lfs_file_read(&lfs, &file[0], buffer, size) => size;
                memcmp(buffer, "\0\0\0\0", size) => 0;
            }

            lfs_file_close(&lfs, &file[0]) => 0;
        }
    }
    lfs_unmount(&lfs) => 0;
TEST
}

printf "--- Cold shrinking truncate ---\n"
truncate_test \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE"

printf "--- Cold expanding truncate ---\n"
truncate_test \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE"

printf "--- Warm shrinking truncate ---\n"
truncate_test \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "           0,            0,            0,            0,            0"

printf "--- Warm expanding truncate ---\n"
truncate_test \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE"

printf "--- Mid-file shrinking truncate ---\n"
truncate_test \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "  $LARGESIZE,   $LARGESIZE,   $LARGESIZE,   $LARGESIZE,   $LARGESIZE" \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "           0,            0,            0,            0,            0"

printf "--- Mid-file expanding truncate ---\n"
truncate_test \
    "           0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE, 2*$LARGESIZE" \
    "           0,            0,   $SMALLSIZE,  $MEDIUMSIZE,   $LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE" \
    "2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE, 2*$LARGESIZE"

printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
##### test_parallel.sh ######
#############################
printf "=== Parallel tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

printf "--- Parallel file test ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "a", LFS_O_WRONLY | LFS_O_CREAT) => 0;
    lfs_file_open(&lfs, &file[1], "b", LFS_O_WRONLY | LFS_O_CREAT) => 0;
    lfs_file_open(&lfs, &file[2], "c", LFS_O_WRONLY | LFS_O_CREAT) => 0;
    lfs_file_open(&lfs, &file[3], "d", LFS_O_WRONLY | LFS_O_CREAT) => 0;

    for (int i = 0; i < 10; i++) {
        lfs_file_write(&lfs, &file[0], (const void*)"a", 1) => 1;
        lfs_file_write(&lfs, &file[1], (const void*)"b", 1) => 1;
        lfs_file_write(&lfs, &file[2], (const void*)"c", 1) => 1;
        lfs_file_write(&lfs, &file[3], (const void*)"d", 1) => 1;
    }

    lfs_file_close(&lfs, &file[0]);
    lfs_file_close(&lfs, &file[1]);
    lfs_file_close(&lfs, &file[2]);
    lfs_file_close(&lfs, &file[3]);

    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "a") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "b") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "c") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "d") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;

    lfs_file_open(&lfs, &file[0], "a", LFS_O_RDONLY) => 0;
    lfs_file_open(&lfs, &file[1], "b", LFS_O_RDONLY) => 0;
    lfs_file_open(&lfs, &file[2], "c", LFS_O_RDONLY) => 0;
    lfs_file_open(&lfs, &file[3], "d", LFS_O_RDONLY) => 0;

    for (int i = 0; i < 10; i++) {
        lfs_file_read(&lfs, &file[0], buffer, 1) => 1;
        buffer[0] => 'a';
        lfs_file_read(&lfs, &file[1], buffer, 1) => 1;
        buffer[0] => 'b';
        lfs_file_read(&lfs, &file[2], buffer, 1) => 1;
        buffer[0] => 'c';
        lfs_file_read(&lfs, &file[3], buffer, 1) => 1;
        buffer[0] => 'd';
    }

    lfs_file_close(&lfs, &file[0]);
    lfs_file_close(&lfs, &file[1]);
    lfs_file_close(&lfs, &file[2]);
    lfs_file_close(&lfs, &file[3]);
    
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Parallel remove file test ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "e", LFS_O_WRONLY | LFS_O_CREAT) => 0;

    for (int i = 0; i < 5; i++) {
        lfs_file_write(&lfs, &file[0], (const void*)"e", 1) => 1;
    }

    lfs_remove(&lfs, "a") => 0;
    lfs_remove(&lfs, "b") => 0;
    lfs_remove(&lfs, "c") => 0;
    lfs_remove(&lfs, "d") => 0;

    for (int i = 0; i < 5; i++) {
        lfs_file_write(&lfs, &file[0], (const void*)"e", 1) => 1;
    }

    lfs_file_close(&lfs, &file[0]);

    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "e") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;

    lfs_file_open(&lfs, &file[0], "e", LFS_O_RDONLY) => 0;

    for (int i = 0; i < 10; i++) {
        lfs_file_read(&lfs, &file[0], buffer, 1) => 1;
        buffer[0] => 'e';
    }

    lfs_file_close(&lfs, &file[0]);
    
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Remove inconveniently test ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "e", LFS_O_WRONLY | LFS_O_TRUNC) => 0;
    lfs_file_open(&lfs, &file[1], "f", LFS_O_WRONLY | LFS_O_CREAT) => 0;
    lfs_file_open(&lfs, &file[2], "g", LFS_O_WRONLY | LFS_O_CREAT) => 0;

    for (int i = 0; i < 5; i++) {
        lfs_file_write(&lfs, &file[0], (const void*)"e", 1) => 1;
        lfs_file_write(&lfs, &file[1], (const void*)"f", 1) => 1;
        lfs_file_write(&lfs, &file[2], (const void*)"g", 1) => 1;
    }

    lfs_remove(&lfs, "f") => 0;

    for (int i = 0; i < 5; i++) {
        lfs_file_write(&lfs, &file[0], (const void*)"e", 1) => 1;
        lfs_file_write(&lfs, &file[1], (const void*)"f", 1) => 1;
        lfs_file_write(&lfs, &file[2], (const void*)"g", 1) => 1;
    }

    lfs_file_close(&lfs, &file[0]);
    lfs_file_close(&lfs, &file[1]);
    lfs_file_close(&lfs, &file[2]);

    lfs_dir_open(&lfs, &dir[0], "/") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    info.type => LFS_TYPE_DIR;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "e") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "g") => 0;
    info.type => LFS_TYPE_REG;
    info.size => 10;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;

    lfs_file_open(&lfs, &file[0], "e", LFS_O_RDONLY) => 0;
    lfs_file_open(&lfs, &file[1], "g", LFS_O_RDONLY) => 0;

    for (int i = 0; i < 10; i++) {
        lfs_file_read(&lfs, &file[0], buffer, 1) => 1;
        buffer[0] => 'e';
        lfs_file_read(&lfs, &file[1], buffer, 1) => 1;
        buffer[0] => 'g';
    }

    lfs_file_close(&lfs, &file[0]);
    lfs_file_close(&lfs, &file[1]);
    
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
####### test_alloc.sh #######
#############################

printf "=== Allocator tests ==="
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

SIZE=15000

lfs_mkdir() {
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "$1") => 0;
    lfs_unmount(&lfs) => 0;
TEST
}

lfs_remove() {
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "$1/eggs") => 0;
    lfs_remove(&lfs, "$1/bacon") => 0;
    lfs_remove(&lfs, "$1/pancakes") => 0;
    lfs_remove(&lfs, "$1") => 0;
    lfs_unmount(&lfs) => 0;
TEST
}

lfs_alloc_singleproc() {
cat >> $OUTPUTFILE << TEST
        {
            const char *names[] = {"bacon", "eggs", "pancakes"};
            lfs_mount(&lfs, &cfg) => 0;
            for (int n = 0; n < sizeof(names)/sizeof(names[0]); n++) {
                sprintf((char*)buffer, "$1/%s", names[n]);
                lfs_file_open(&lfs, &file[n], (char*)buffer,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) => 0;
            }
            for (int n = 0; n < sizeof(names)/sizeof(names[0]); n++) {
                size = strlen(names[n]);
                for (int i = 0; i < $SIZE; i++) {
                    lfs_file_write(&lfs, &file[n], names[n], size) => size;
                }
            }
            for (int n = 0; n < sizeof(names)/sizeof(names[0]); n++) {
                lfs_file_close(&lfs, &file[n]) => 0;
            }
            lfs_unmount(&lfs) => 0;
        }
TEST
}

lfs_alloc_multiproc() {
for name in bacon eggs pancakes
do
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "$1/$name",
            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) => 0;
    size = strlen("$name");
    memcpy(buffer, "$name", size);
    for (int i = 0; i < $SIZE; i++) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST
done
}

lfs_verify() {
for name in bacon eggs pancakes
do
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "$1/$name", LFS_O_RDONLY) => 0;
    size = strlen("$name");
    for (int i = 0; i < $SIZE; i++) {
        lfs_file_read(&lfs, &file[0], buffer, size) => size;
        memcmp(buffer, "$name", size) => 0;
    }
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST
done
}

printf "--- Single-process allocation test ---"
lfs_mkdir singleproc
lfs_alloc_singleproc singleproc
lfs_verify singleproc

printf "--- Multi-process allocation test ---"
lfs_mkdir multiproc
lfs_alloc_multiproc multiproc
lfs_verify multiproc
lfs_verify singleproc

printf "--- Single-process reuse test ---"
lfs_remove singleproc
lfs_mkdir singleprocreuse
lfs_alloc_singleproc singleprocreuse
lfs_verify singleprocreuse
lfs_verify multiproc

printf "--- Multi-process reuse test ---"
lfs_remove multiproc
lfs_mkdir multiprocreuse
lfs_alloc_singleproc multiprocreuse
lfs_verify multiprocreuse
lfs_verify singleprocreuse

printf "--- Cleanup ---"
lfs_remove multiprocreuse
lfs_remove singleprocreuse

printf "--- Exhaustion test ---"
cat >> $OUTPUTFILE << TEST
    {
        lfs_mount(&lfs, &cfg) => 0;
        lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_CREAT);
        size = strlen("exhaustion");
        memcpy(buffer, "exhaustion", size);
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
        lfs_file_sync(&lfs, &file[0]) => 0;

        size = strlen("blahblahblahblah");
        memcpy(buffer, "blahblahblahblah", size);
        lfs_ssize_t res;
        while (true) {
            res = lfs_file_write(&lfs, &file[0], buffer, size);
            if (res < 0) {
                break;
            }

            res => size;
        }
        res => LFS_ERR_NOSPC;

        lfs_file_close(&lfs, &file[0]) => 0;
        lfs_unmount(&lfs) => 0;
    }
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_RDONLY);
    size = strlen("exhaustion");
    lfs_file_size(&lfs, &file[0]) => size;
    lfs_file_read(&lfs, &file[0], buffer, size) => size;
    memcmp(buffer, "exhaustion", size) => 0;
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Exhaustion wraparound test ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "exhaustion") => 0;

    lfs_file_open(&lfs, &file[0], "padding", LFS_O_WRONLY | LFS_O_CREAT);
    size = strlen("buffering");
    memcpy(buffer, "buffering", size);
    for (int i = 0; i < $SIZE; i++) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_remove(&lfs, "padding") => 0;

    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_CREAT);
    size = strlen("exhaustion");
    memcpy(buffer, "exhaustion", size);
    lfs_file_write(&lfs, &file[0], buffer, size) => size;
    lfs_file_sync(&lfs, &file[0]) => 0;

    size = strlen("blahblahblahblah");
    memcpy(buffer, "blahblahblahblah", size);
    lfs_ssize_t res;
    while (true) {
        res = lfs_file_write(&lfs, &file[0], buffer, size);
        if (res < 0) {
            break;
        }

        res => size;
    }
    res => LFS_ERR_NOSPC;

    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_RDONLY);
    size = strlen("exhaustion");
    lfs_file_size(&lfs, &file[0]) => size;
    lfs_file_read(&lfs, &file[0], buffer, size) => size;
    memcmp(buffer, "exhaustion", size) => 0;
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Dir exhaustion test ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "exhaustion") => 0;

    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_CREAT);
    size = strlen("blahblahblahblah");
    memcpy(buffer, "blahblahblahblah", size);
    for (lfs_size_t i = 0;
            i < (cfg.block_count-6)*(cfg.block_size-8);
            i += size) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_mkdir(&lfs, "exhaustiondir") => 0;
    lfs_remove(&lfs, "exhaustiondir") => 0;

    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_APPEND);
    size = strlen("blahblahblahblah");
    memcpy(buffer, "blahblahblahblah", size);
    for (lfs_size_t i = 0;
            i < (cfg.block_size-8);
            i += size) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_mkdir(&lfs, "exhaustiondir") => LFS_ERR_NOSPC;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Chained dir exhaustion test ---"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_remove(&lfs, "exhaustion") => 0;

    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_CREAT);
    size = strlen("blahblahblahblah");
    memcpy(buffer, "blahblahblahblah", size);
    for (lfs_size_t i = 0;
            i < (cfg.block_count-24)*(cfg.block_size-8);
            i += size) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;

    for (int i = 0; i < 9; i++) {
        sprintf((char*)buffer, "dirwithanexhaustivelylongnameforpadding%d", i);
        lfs_mkdir(&lfs, (char*)buffer) => 0;
    }

    lfs_mkdir(&lfs, "exhaustiondir") => LFS_ERR_NOSPC;

    lfs_remove(&lfs, "exhaustion") => 0;
    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_CREAT);
    size = strlen("blahblahblahblah");
    memcpy(buffer, "blahblahblahblah", size);
    for (lfs_size_t i = 0;
            i < (cfg.block_count-26)*(cfg.block_size-8);
            i += size) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_mkdir(&lfs, "exhaustiondir") => 0;
    lfs_mkdir(&lfs, "exhaustiondir2") => LFS_ERR_NOSPC;
TEST

printf "--- Split dir test ---"
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;

    // create one block whole for half a directory
    lfs_file_open(&lfs, &file[0], "bump", LFS_O_WRONLY | LFS_O_CREAT) => 0;
    lfs_file_write(&lfs, &file[0], (void*)"hi", 2) => 2;
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_file_open(&lfs, &file[0], "exhaustion", LFS_O_WRONLY | LFS_O_CREAT);
    size = strlen("blahblahblahblah");
    memcpy(buffer, "blahblahblahblah", size);
    for (lfs_size_t i = 0;
            i < (cfg.block_count-6)*(cfg.block_size-8);
            i += size) {
        lfs_file_write(&lfs, &file[0], buffer, size) => size;
    }
    lfs_file_close(&lfs, &file[0]) => 0;

    // open whole
    lfs_remove(&lfs, "bump") => 0;

    lfs_mkdir(&lfs, "splitdir") => 0;
    lfs_file_open(&lfs, &file[0], "splitdir/bump",
            LFS_O_WRONLY | LFS_O_CREAT) => 0;
    lfs_file_write(&lfs, &file[0], buffer, size) => LFS_ERR_NOSPC;
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_unmount(&lfs) => 0;
TEST

printf "--- Results ---"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
####### test_paths.sh #######
#############################
printf "=== Path tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "tea") => 0;
    lfs_mkdir(&lfs, "coffee") => 0;
    lfs_mkdir(&lfs, "soda") => 0;
    lfs_mkdir(&lfs, "tea/hottea") => 0;
    lfs_mkdir(&lfs, "tea/warmtea") => 0;
    lfs_mkdir(&lfs, "tea/coldtea") => 0;
    lfs_mkdir(&lfs, "coffee/hotcoffee") => 0;
    lfs_mkdir(&lfs, "coffee/warmcoffee") => 0;
    lfs_mkdir(&lfs, "coffee/coldcoffee") => 0;
    lfs_mkdir(&lfs, "soda/hotsoda") => 0;
    lfs_mkdir(&lfs, "soda/warmsoda") => 0;
    lfs_mkdir(&lfs, "soda/coldsoda") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Root path tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_stat(&lfs, "tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "/tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;

    lfs_mkdir(&lfs, "/milk1") => 0;
    lfs_stat(&lfs, "/milk1", &info) => 0;
    strcmp(info.name, "milk1") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Redundant slash path tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_stat(&lfs, "/tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "//tea//hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "///tea///hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;

    lfs_mkdir(&lfs, "///milk2") => 0;
    lfs_stat(&lfs, "///milk2", &info) => 0;
    strcmp(info.name, "milk2") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Dot path tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_stat(&lfs, "./tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "/./tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "/././tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "/./tea/./hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;

    lfs_mkdir(&lfs, "/./milk3") => 0;
    lfs_stat(&lfs, "/./milk3", &info) => 0;
    strcmp(info.name, "milk3") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Dot dot path tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_stat(&lfs, "coffee/../tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "tea/coldtea/../hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "coffee/coldcoffee/../../tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;
    lfs_stat(&lfs, "coffee/../soda/../tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;

    lfs_mkdir(&lfs, "coffee/../milk4") => 0;
    lfs_stat(&lfs, "coffee/../milk4", &info) => 0;
    strcmp(info.name, "milk4") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Root dot dot path tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_stat(&lfs, "coffee/../../../../../../tea/hottea", &info) => 0;
    strcmp(info.name, "hottea") => 0;

    lfs_mkdir(&lfs, "coffee/../../../../../../milk5") => 0;
    lfs_stat(&lfs, "coffee/../../../../../../milk5", &info) => 0;
    strcmp(info.name, "milk5") => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Root tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_stat(&lfs, "/", &info) => 0;
    info.type => LFS_TYPE_DIR;
    strcmp(info.name, "/") => 0;

    lfs_mkdir(&lfs, "/") => LFS_ERR_EXIST;
    lfs_file_open(&lfs, &file[0], "/", LFS_O_WRONLY | LFS_O_CREAT)
        => LFS_ERR_ISDIR;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Sketchy path tests ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "dirt/ground") => LFS_ERR_NOENT;
    lfs_mkdir(&lfs, "dirt/ground/earth") => LFS_ERR_NOENT;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Results ---\n"


#############################
####### test_orphan.sh ######
#############################
printf "=== Orphan tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;
TEST

printf "--- Orphan test ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "parent") => 0;
    lfs_mkdir(&lfs, "parent/orphan") => 0;
    lfs_mkdir(&lfs, "parent/child") => 0;
    lfs_remove(&lfs, "parent/orphan") => 0;
TEST
# remove most recent file, this should be the update to the previous
# linked-list entry and should orphan the child
# TODO: this needs an equivalent
# rm -v blocks/8
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => 0;
#     lfs_stat(&lfs, "parent/orphan", &info) => LFS_ERR_NOENT;
#     unsigned before = 0;
#     lfs_traverse(&lfs, test_count, &before) => 0;
#     test_log("before", before);

#     lfs_deorphan(&lfs) => 0;

#     lfs_stat(&lfs, "parent/orphan", &info) => LFS_ERR_NOENT;
#     unsigned after = 0;
#     lfs_traverse(&lfs, test_count, &after) => 0;
#     test_log("after", after);

#     int diff = before - after;
#     diff => 2;
#     lfs_unmount(&lfs) => 0;
# TEST

printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"



#############################
####### test_move.sh ########
#############################
printf "=== Move tests ===\n"
# rm -rf blocks
cat >> $OUTPUTFILE << TEST
    lfs_format(&lfs, &cfg) => 0;

    lfs_mount(&lfs, &cfg) => 0;
    lfs_mkdir(&lfs, "a") => 0;
    lfs_mkdir(&lfs, "b") => 0;
    lfs_mkdir(&lfs, "c") => 0;
    lfs_mkdir(&lfs, "d") => 0;

    lfs_mkdir(&lfs, "a/hi") => 0;
    lfs_mkdir(&lfs, "a/hi/hola") => 0;
    lfs_mkdir(&lfs, "a/hi/bonjour") => 0;
    lfs_mkdir(&lfs, "a/hi/ohayo") => 0;

    lfs_file_open(&lfs, &file[0], "a/hello", LFS_O_CREAT | LFS_O_WRONLY) => 0;
    lfs_file_write(&lfs, &file[0], "hola\n", 5) => 5;
    lfs_file_write(&lfs, &file[0], "bonjour\n", 8) => 8;
    lfs_file_write(&lfs, &file[0], "ohayo\n", 6) => 6;
    lfs_file_close(&lfs, &file[0]) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Move file ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "a/hello", "b/hello") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "a") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "hi") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_dir_open(&lfs, &dir[0], "b") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "hello") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Move file corrupt source ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "b/hello", "c/hello") => 0;
    lfs_unmount(&lfs) => 0;
TEST
# TODO: This needs an equivalent
# rm -v blocks/7
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => 0;
#     lfs_dir_open(&lfs, &dir[0], "b") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_dir_close(&lfs, &dir[0]) => 0;
#     lfs_dir_open(&lfs, &dir[0], "c") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "hello") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_unmount(&lfs) => 0;
# TEST

printf "--- Move file corrupt source and dest ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "c/hello", "d/hello") => 0;
    lfs_unmount(&lfs) => 0;
TEST
# TODO: This needs an equivalent
# rm -v blocks/8
# rm -v blocks/a
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => 0;
#     lfs_dir_open(&lfs, &dir[0], "c") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "hello") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_dir_close(&lfs, &dir[0]) => 0;
#     lfs_dir_open(&lfs, &dir[0], "d") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_unmount(&lfs) => 0;
# TEST

printf "--- Move dir ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "a/hi", "b/hi") => 0;
    lfs_unmount(&lfs) => 0;
TEST
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_dir_open(&lfs, &dir[0], "a") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;
    lfs_dir_open(&lfs, &dir[0], "b") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "hi") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_unmount(&lfs) => 0;
TEST

printf "--- Move dir corrupt source ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "b/hi", "c/hi") => 0;
    lfs_unmount(&lfs) => 0;
TEST
# TODO: This needs an equivalent
# rm -v blocks/7
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => 0;
#     lfs_dir_open(&lfs, &dir[0], "b") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_dir_close(&lfs, &dir[0]) => 0;
#     lfs_dir_open(&lfs, &dir[0], "c") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "hello") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "hi") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_unmount(&lfs) => 0;
# TEST

printf "--- Move dir corrupt source and dest ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;
    lfs_rename(&lfs, "c/hi", "d/hi") => 0;
    lfs_unmount(&lfs) => 0;
TEST
# TODO: This needs an equivalent
# rm -v blocks/9
# rm -v blocks/a
# cat >> $OUTPUTFILE << TEST
#     lfs_mount(&lfs, &cfg) => 0;
#     lfs_dir_open(&lfs, &dir[0], "c") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "hello") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "hi") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_dir_close(&lfs, &dir[0]) => 0;
#     lfs_dir_open(&lfs, &dir[0], "d") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, ".") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 1;
#     strcmp(info.name, "..") => 0;
#     lfs_dir_read(&lfs, &dir[0], &info) => 0;
#     lfs_unmount(&lfs) => 0;
# TEST

printf "--- Move check ---\n"
cat >> $OUTPUTFILE << TEST
    lfs_mount(&lfs, &cfg) => 0;

    lfs_dir_open(&lfs, &dir[0], "a/hi") => LFS_ERR_NOENT;
    lfs_dir_open(&lfs, &dir[0], "b/hi") => LFS_ERR_NOENT;
    lfs_dir_open(&lfs, &dir[0], "d/hi") => LFS_ERR_NOENT;

    lfs_dir_open(&lfs, &dir[0], "c/hi") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, ".") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "..") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "hola") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "bonjour") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 1;
    strcmp(info.name, "ohayo") => 0;
    lfs_dir_read(&lfs, &dir[0], &info) => 0;
    lfs_dir_close(&lfs, &dir[0]) => 0;

    lfs_dir_open(&lfs, &dir[0], "a/hello") => LFS_ERR_NOENT;
    lfs_dir_open(&lfs, &dir[0], "b/hello") => LFS_ERR_NOENT;
    lfs_dir_open(&lfs, &dir[0], "d/hello") => LFS_ERR_NOENT;

    lfs_file_open(&lfs, &file[0], "c/hello", LFS_O_RDONLY) => 0;
    lfs_file_read(&lfs, &file[0], buffer, 5) => 5;
    memcmp(buffer, "hola\n", 5) => 0;
    lfs_file_read(&lfs, &file[0], buffer, 8) => 8;
    memcmp(buffer, "bonjour\n", 8) => 0;
    lfs_file_read(&lfs, &file[0], buffer, 6) => 6;
    memcmp(buffer, "ohayo\n", 6) => 0;
    lfs_file_close(&lfs, &file[0]) => 0;

    lfs_unmount(&lfs) => 0;
TEST


printf "--- Results ---\n"
printf "IMPLEMENT RESULTS FUNCTION HERE"




###############################
####### test_corrupt.sh #######
###############################

# don't know how to do this yet







