#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_client.h"
#include <oss_c_sdk/aos_define.h>

int64_t write_file(const char* content);
void delete_file(oss_media_file_t *file);
static void auth_func(oss_media_file_t *file);

void test_client_setup(CuTest *tc) {
    aos_pool_t *p;
    aos_pool_create(&p, NULL);
    
    apr_dir_make(TEST_DIR"/data/", APR_OS_DEFAULT, p);

    aos_pool_destroy(p);
}

void test_client_teardown(CuTest *tc) {
    aos_pool_t *p;
    aos_pool_create(&p, NULL);
    apr_dir_remove(TEST_DIR"/data/", p);
    aos_pool_destroy(p);    
}

void test_write_file_succeeded(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // create file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "w", auth_func);
    CuAssertTrue(tc, NULL != file);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Normal", stat.type);
    CuAssertIntEquals(tc, write_size, stat.length);

    delete_file(file);

    // close file
    oss_media_file_close(file);
}

void test_append_file_succeeded(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file.txt", 
                               "a", auth_func);
    CuAssertTrue(tc, NULL != file);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, strlen(content), write_size);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, strlen(content), write_size);

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Appendable", stat.type);
    CuAssertIntEquals(tc, write_size * 2, stat.length);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_write_file_failed_with_invalid_key(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    char *file_name = NULL;

    content = "hello oss media file\n";
    file_name = "\\invalid.key";

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "w", auth_func);
    CuAssertTrue(tc, NULL != file);

    file->object_key = file_name;;

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, -1, write_size);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}


void test_write_file_failed_with_wrong_flag(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;

    content = "hello oss media file\n";

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, -1, write_size);    

    // close file
    oss_media_file_close(file);
}

void test_write_file_with_normal_cover_appendable(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file.txt", 
                               "a", auth_func);
    CuAssertTrue(tc, NULL != file);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    // close file 
    oss_media_file_close(file);

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file.txt", 
                               "w", auth_func);
    CuAssertTrue(tc, NULL != file);

    write_size = oss_media_file_write(file, content, strlen(content) - 2);
    CuAssertIntEquals(tc, strlen(content) - 2, write_size);

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Normal", stat.type);
    CuAssertIntEquals(tc, strlen(content) - 2, stat.length);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
}

void test_append_file_failed_with_appendable_cover_normal(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "w", auth_func);
    CuAssertTrue(tc, NULL != file);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    // close file
    oss_media_file_close(file);

    // open file
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "a", auth_func);
    CuAssertTrue(tc, NULL != file);

    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, -1, write_size);

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Normal", stat.type);
    CuAssertIntEquals(tc, strlen(content), stat.length);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
}

void test_read_file_succeeded(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);
    CuAssertTrue(tc, write_size != -1);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    // read file
    read_content = malloc(write_size + 1);
    ntotal = 0;
    while ((nread = oss_media_file_read(file, buf, nbuf)) > 0) {
        memcpy(read_content + ntotal, buf, nread);
        ntotal += nread;
    }
    read_content[ntotal] = '\0';    

    CuAssertIntEquals(tc, ntotal, strlen(read_content));
    CuAssertStrEquals(tc, write_content, read_content);

    // close file
    free(read_content);
    delete_file(file);
    oss_media_file_close(file);
}

void test_read_part_file_succeeded(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    int nread = 4;
    char buf[100];
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    // check tell
    CuAssertIntEquals(tc, 0, oss_media_file_tell(file));

    // read file
    read_content = malloc(write_size / 2 + 1);

    nread = oss_media_file_read(file, buf, write_size / 2);
    memcpy(read_content, buf, nread);
    read_content[nread] = '\0';    

    CuAssertIntEquals(tc, nread, strlen(read_content));
    CuAssertStrEquals(tc, "hello oss ", read_content);

    // check tell again
    CuAssertIntEquals(tc, write_size / 2, oss_media_file_tell(file));

    free(read_content);

    read_content = malloc(write_size / 2 + 1);
    oss_media_file_seek(file, write_size / 2);
    nread = oss_media_file_read(file, buf, write_size / 2);
    memcpy(read_content, buf, nread);
    read_content[nread] = '\0';    

    CuAssertIntEquals(tc, nread, strlen(read_content));
    CuAssertStrEquals(tc, "media file", read_content);    

    // check tell again
    CuAssertIntEquals(tc, write_size - 1, oss_media_file_tell(file));

    // close file and free
    free(read_content);
    delete_file(file);
    oss_media_file_close(file);
}

void test_read_file_failed_with_wrong_flag(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    int nread, nbuf = 4;
    char buf[4];
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "w", auth_func);
    CuAssertTrue(tc, NULL != file);

    // read file
    nread = oss_media_file_read(file, buf, nbuf);

    CuAssertIntEquals(tc, -1, nread);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_read_file_failed_with_eof(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    int nread, nbuf = 4;
    char buf[4];
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    CuAssertIntEquals(tc, write_size - 1, oss_media_file_seek(file, write_size -1));
    CuAssertIntEquals(tc, write_size - 1, oss_media_file_tell(file));

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, 1, nread);

    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, 0, nread);

    CuAssertIntEquals(tc, -1, oss_media_file_seek(file, write_size + 1));
    CuAssertIntEquals(tc, write_size, oss_media_file_tell(file));

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, 0, nread);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_read_file_failed_with_key_is_not_exist(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    int nread, nbuf = 4;
    char buf[4];
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    file->object_key = "not_exist.key";

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, -1, nread);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_read_file_failed_with_invalid_key(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    int nread, nbuf = 4;
    char buf[4];
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    file->object_key = "//invalid.key";

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, -1, nread);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_seek_file_failed_with_invalid_pos(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    // seek file
    ret = oss_media_file_seek(file, -1);
    CuAssertIntEquals(tc, -1, ret);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_seek_file_failed_with_file_not_exist(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    oss_media_file_stat_t stat;
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "r", auth_func);
    CuAssertTrue(tc, NULL != file);

    file->object_key = "not_exist.key";

    // seek file
    ret = oss_media_file_seek(file, 1);
    CuAssertIntEquals(tc, -1, ret);

    // tell file
    ret = oss_media_file_tell(file);
    CuAssertIntEquals(tc, -1, ret);

    // stat file
    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, -1, ret);

    // close file
    delete_file(file);
    oss_media_file_close(file);
}

void test_open_file_failed_with_wrong_flag(CuTest *tc) {
    oss_media_file_t *file = NULL;

    // open file for read
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "x", auth_func);
    CuAssertTrue(tc, NULL == file);
}

static void auth_func(oss_media_file_t *file) {
    file->endpoint = TEST_OSS_ENDPOINT;
    file->is_cname = 0;
    file->access_key_id = TEST_ACCESS_KEY_ID;
    file->access_key_secret = TEST_ACCESS_KEY_SECRET;
    file->token = NULL; //SAMPLE_STS_TOKEN; // if use sts token

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

int64_t write_file(const char* content) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;

    // open file for write
    file = oss_media_file_open(TEST_BUCKET_NAME, "oss_media_file", "w", auth_func);
    if (NULL == file)
        return -1;

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    
    // close file
    oss_media_file_close(file);

    return write_size;
}

void delete_file(oss_media_file_t *file) {
    aos_pool_t *p;
    aos_string_t bucket;
    aos_string_t object;
    oss_request_options_t *options;
    aos_table_t *resp_headers;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    aos_str_set(&options->config->endpoint, file->endpoint);
    aos_str_set(&options->config->access_key_id, file->access_key_id);
    aos_str_set(&options->config->access_key_secret, file->access_key_secret);
    options->config->is_cname = file->is_cname;
    options->ctl = aos_http_controller_create(options->pool, 0);    
    aos_str_set(&bucket, file->bucket_name);
    aos_str_set(&object, file->object_key);

    oss_delete_object(options, &bucket, &object, &resp_headers);
}

CuSuite *test_client()
{
    CuSuite* suite = CuSuiteNew();   

    SUITE_ADD_TEST(suite, test_client_setup);

    // write test
    SUITE_ADD_TEST(suite, test_write_file_succeeded);
    SUITE_ADD_TEST(suite, test_append_file_succeeded);
    SUITE_ADD_TEST(suite, test_write_file_failed_with_wrong_flag);
    SUITE_ADD_TEST(suite, test_write_file_with_normal_cover_appendable);
    SUITE_ADD_TEST(suite, test_append_file_failed_with_appendable_cover_normal);
    SUITE_ADD_TEST(suite, test_write_file_failed_with_invalid_key);
    
    // read test
    SUITE_ADD_TEST(suite, test_read_file_succeeded);
    SUITE_ADD_TEST(suite, test_read_part_file_succeeded);
    SUITE_ADD_TEST(suite, test_read_file_failed_with_wrong_flag);
    SUITE_ADD_TEST(suite, test_read_file_failed_with_eof);
    SUITE_ADD_TEST(suite, test_read_file_failed_with_key_is_not_exist);
    SUITE_ADD_TEST(suite, test_read_file_failed_with_invalid_key);

    //seek / tell / stat test
    SUITE_ADD_TEST(suite, test_seek_file_failed_with_invalid_pos);

    // open test
    SUITE_ADD_TEST(suite, test_open_file_failed_with_wrong_flag);

    SUITE_ADD_TEST(suite, test_client_teardown); 
    
    return suite;
}
