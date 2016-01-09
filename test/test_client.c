#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_client.h"
#include <oss_c_sdk/aos_define.h>
#include <oss_c_sdk/aos_status.h>

static int64_t write_file(const char* content);
static void auth_func(oss_media_file_t *file);
static aos_status_t* delete_file(oss_media_file_t *file);

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
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "w");
    CuAssertIntEquals(tc, 0, ret);

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

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);
}

void test_append_file_succeeded(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "a");
    CuAssertIntEquals(tc, 0, ret);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Appendable", stat.type);
    CuAssertIntEquals(tc, write_size * 2, stat.length);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);
}

void test_write_file_failed_with_invalid_key(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    char *file_name = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";
    file_name = "\\invalid.key";

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "w");
    CuAssertIntEquals(tc, 0, ret);

    file->filename = file_name;;

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, -1, write_size);

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, -1, ret);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);
}


void test_write_file_failed_with_wrong_flag(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, -1, write_size);    

    // close file and free
    oss_media_file_free(file);
}

void test_write_file_with_normal_cover_appendable(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "a");
    CuAssertIntEquals(tc, 0, ret);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "w");
    CuAssertIntEquals(tc, 0, ret);

    write_size = oss_media_file_write(file, content, strlen(content) - 2);
    CuAssertIntEquals(tc, strlen(content) - 2, write_size);

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Normal", stat.type);
    CuAssertIntEquals(tc, strlen(content) - 2, stat.length);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);
}

void test_append_file_failed_with_appendable_cover_normal(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *content = NULL;
    oss_media_file_stat_t stat;
    int ret;

    content = "hello oss media file\n";

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "w");
    CuAssertIntEquals(tc, 0, ret);

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, write_size, strlen(content));

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    ret = oss_media_file_open(file, "a");
    CuAssertIntEquals(tc, 0, ret);

    write_size = oss_media_file_write(file, content, strlen(content));
    CuAssertIntEquals(tc, -1, write_size);

    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, "Normal", stat.type);
    CuAssertIntEquals(tc, strlen(content), stat.length);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);
}

void test_read_file_succeeded(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);
    CuAssertTrue(tc, write_size != -1);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

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

    // close file and free
    free(read_content);
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_read_part_file_succeeded(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[100];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

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
    oss_media_file_free(file);    
}

void test_read_file_failed_with_wrong_flag(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "w");
    CuAssertIntEquals(tc, 0, ret);

    // read file
    nread = oss_media_file_read(file, buf, nbuf);

    CuAssertIntEquals(tc, -1, nread);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_read_file_failed_with_eof(CuTest *tc) {
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

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

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_read_file_failed_with_key_is_not_exist(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

    file->filename = "not_exist.key";

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, -1, nread);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_read_file_failed_with_invalid_key(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

    file->filename = "//invalid.key";

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, -1, nread);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_seek_file_failed_with_invalid_pos(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

    // seek file
    ret = oss_media_file_seek(file, -1);
    CuAssertIntEquals(tc, -1, ret);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_seek_file_failed_with_file_not_exist(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    oss_media_file_stat_t stat;
    char *read_content = NULL;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    write_size = write_file(write_content);

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

    file->filename = "not_exist.key";

    // seek file
    ret = oss_media_file_seek(file, 1);
    CuAssertIntEquals(tc, -1, ret);

    // tell file
    ret = oss_media_file_tell(file);
    CuAssertIntEquals(tc, -1, ret);

    // stat file
    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, -1, ret);

    // close file and free
    delete_file(file);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

void test_open_file_failed_with_wrong_flag(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    oss_media_file_stat_t stat;
    char *read_content = NULL;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "x");
    CuAssertIntEquals(tc, -1, ret);

    // close file and free
    oss_media_file_free(file);    
}

void test_expiration_early_than_current(CuTest *tc) {
    int64_t read_size = 0;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;
    char *write_content = NULL;
    char *read_content = NULL;
    oss_media_file_stat_t stat;
    int ntotal, nread, nbuf = 4;
    char buf[4];
    int ret;
    
    write_content = "hello oss media file\n";
    {
        // create file
        file = oss_media_file_create();
        file->auth = auth_func;
        file->expiration = time(NULL) - 100;

        // open file for write
        ret = oss_media_file_open(file, "w");
        CuAssertIntEquals(tc, 0, ret);

        // write file
        write_size = oss_media_file_write(file, write_content, 
                strlen(write_content));
        CuAssertIntEquals(tc, write_size, strlen(write_content));

        // close file and free
        delete_file(file);
        oss_media_file_close(file);
        oss_media_file_free(file);
    }

    // create file for read
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for read
    ret = oss_media_file_open(file, "r");
    CuAssertIntEquals(tc, 0, ret);

    // read file
    nread = oss_media_file_read(file, buf, nbuf);
    CuAssertIntEquals(tc, 0, nread);

    // stat file
    ret = oss_media_file_stat(file, &stat);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 0, stat.length);
    CuAssertStrEquals(tc, OSS_MEDIA_FILE_UNKNOWN_TYPE, stat.type);

    // close file and free
    delete_file(file);
    free(read_content);
    oss_media_file_close(file);
    oss_media_file_free(file);    
}

static int64_t write_file(const char* content) {
    int ret;
    int64_t write_size = 0;
    oss_media_file_t *file = NULL;

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file for write
    ret = oss_media_file_open(file, "w");
    if (ret != 0)
        return -1;

    // write file
    write_size = oss_media_file_write(file, content, strlen(content));

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);

    return write_size;
}

static void auth_func(oss_media_file_t *file) {
    file->host = TEST_OSS_HOST;
    file->is_oss_domain = 1; //oss host type, host is oss domain ? 1 : 0(cname etc)
    file->bucket = TEST_BUCKET_NAME;
    file->filename = "oss_media_file";
    file->access_key_id = TEST_ACCESS_KEY_ID;
    file->access_key_secret = TEST_ACCESS_KEY_SECRET;
    file->token = NULL; //SAMPLE_STS_TOKEN; // if use sts token

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

static aos_status_t* delete_file(oss_media_file_t *file) {
    aos_pool_t *p;
    aos_string_t bucket;
    aos_string_t object;
    int is_oss_domain = 1;
    oss_request_options_t *options;
    aos_table_t *resp_headers;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    aos_str_set(&options->config->host, file->host);
    aos_str_set(&options->config->id, file->access_key_id);
    aos_str_set(&options->config->key, file->access_key_secret);
    options->config->is_oss_domain = file->is_oss_domain;
    options->ctl = aos_http_controller_create(options->pool, 0);    
    aos_str_set(&bucket, file->bucket);
    aos_str_set(&object, file->filename);

    return oss_delete_object(options, &bucket, &object, &resp_headers);
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

    //expiration test
    SUITE_ADD_TEST(suite, test_expiration_early_than_current);

    SUITE_ADD_TEST(suite, test_client_teardown); 
    
    return suite;
}
