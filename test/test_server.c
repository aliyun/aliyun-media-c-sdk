#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_server.h"
#include <oss_c_sdk/aos_define.h>
#include <oss_c_sdk/aos_status.h>

static void init_media_config(oss_media_config_t *config);
static int64_t write_file(const char* key, oss_media_config_t *config);

void test_server_setup(CuTest *tc) {
    aos_pool_t *p = NULL;
    aos_pool_create(&p, NULL);
    
    apr_dir_make(TEST_DIR"/data/", APR_OS_DEFAULT, p);

    aos_pool_destroy(p);
}

void test_server_teardown(CuTest *tc) {
    aos_pool_t *p = NULL;
    aos_pool_create(&p, NULL);
    apr_dir_remove(TEST_DIR"/data/", p);
    aos_pool_destroy(p);
}

void test_create_bucket_succeeded(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    char* bucket_name = "test-bucket9";
    
    init_media_config(&config);

    ret = oss_media_create_bucket(&config, bucket_name, 
                                  OSS_ACL_PRIVATE);

    CuAssertIntEquals(tc, 0, ret);

    ret = oss_media_create_bucket(&config, bucket_name, 
                                  OSS_ACL_PUBLIC_READ);

    CuAssertIntEquals(tc, 0, ret);
}

void test_create_bucket_failed(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_create_bucket(&config, "//test", 
                                  OSS_ACL_PRIVATE);

    CuAssertIntEquals(tc, -1, ret);
}

void test_delete_bucket_succeeded(CuTest *tc) {
    int ret;    
    oss_media_config_t config;
    char* bucket_name = "test-bucket9";
    
    init_media_config(&config);

    ret = oss_media_delete_bucket(&config, bucket_name);

    CuAssertIntEquals(tc, 0, ret);    
}

void test_delete_bucket_failed(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    char* bucket_name = "test-bucket9";
    
    init_media_config(&config);

    ret = oss_media_delete_bucket(&config, bucket_name);

    ret = oss_media_delete_bucket(&config, bucket_name);

    CuAssertIntEquals(tc, -1, ret);
}

void test_create_bucket_lifecycle_succeeded(CuTest *tc) {
    int ret;
    oss_media_lifecycle_rules_t *rules = NULL;
    oss_media_config_t config;
    
    init_media_config(&config);

    rules = oss_media_create_lifecycle_rules(2);
    oss_media_lifecycle_rule_t rule1;
    rule1.name = "example-1";
    rule1.path = "/example/1";
    rule1.status = "Enabled";
    rule1.days = 1;

    oss_media_lifecycle_rule_t rule2;
    rule2.name = "example-2";
    rule2.path = "/example/2";
    rule2.status = "Disabled";
    rule2.days = 2;

    rules->rules[0] = &rule1;
    rules->rules[1] = &rule2;

    ret = oss_media_create_bucket_lifecycle(&config, 
            TEST_BUCKET_NAME, rules);
    
    CuAssertIntEquals(tc, 0, ret);

    oss_media_free_lifecycle_rules(rules);
}

void test_create_bucket_lifecycle_failed(CuTest *tc) {
    int ret;
    oss_media_lifecycle_rules_t *rules = NULL;
    oss_media_config_t config;
    
    init_media_config(&config);

    rules = oss_media_create_lifecycle_rules(2);
    oss_media_lifecycle_rule_t rule1;
    rule1.name = "example-1";
    rule1.path = "/example/1";
    rule1.status = "Enabled";
    rule1.days = 1;

    oss_media_lifecycle_rule_t rule2;
    rule2.name = "example-2";
    rule2.path = "/example/2";
    rule2.status = "Disabled";
    rule2.days = 2;

    rules->rules[0] = &rule1;
    rules->rules[1] = &rule2;

    ret = oss_media_create_bucket_lifecycle(&config, 
            "bucket-not-exist", rules);
    
    CuAssertIntEquals(tc, -1, ret);

    oss_media_free_lifecycle_rules(rules);
}

void test_get_bucket_lifecycle_succeeded(CuTest *tc) {
    int ret, i;    
    oss_media_lifecycle_rules_t *rules = NULL;
    oss_media_config_t config;
    
    init_media_config(&config);

    rules = oss_media_create_lifecycle_rules(0);

    sleep(1);
    ret = oss_media_get_bucket_lifecycle(&config, TEST_BUCKET_NAME, rules);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertIntEquals(tc, 2, rules->size);
    CuAssertStrEquals(tc, "example-1", rules->rules[0]->name);
    CuAssertStrEquals(tc, "/example/1", rules->rules[0]->path);
    CuAssertStrEquals(tc, "Enabled", rules->rules[0]->status);
    CuAssertIntEquals(tc, 1, rules->rules[0]->days);
    CuAssertStrEquals(tc, "example-2", rules->rules[1]->name);
    CuAssertStrEquals(tc, "/example/2", rules->rules[1]->path);
    CuAssertStrEquals(tc, "Disabled", rules->rules[1]->status);
    CuAssertIntEquals(tc, 2, rules->rules[1]->days);

    oss_media_free_lifecycle_rules(rules);
}

void test_get_bucket_lifecycle_failed(CuTest *tc) {
    int ret, i;
    oss_media_lifecycle_rules_t *rules = NULL;
    oss_media_config_t config;
    
    init_media_config(&config);

    rules = oss_media_create_lifecycle_rules(0);

    ret = oss_media_get_bucket_lifecycle(&config, 
            "bucket-not-exist", rules);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_free_lifecycle_rules(rules);
}

void test_delete_bucket_lifecycle_succeeded(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_delete_bucket_lifecycle(&config, TEST_BUCKET_NAME);

    CuAssertIntEquals(tc, 0, ret);
}

void test_delete_bucket_lifecycle_failed(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_delete_bucket_lifecycle(&config, 
            "bucket-not-exist");

    CuAssertIntEquals(tc, -1, ret);
}

void test_delete_file_succeeded(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    char *file = NULL;
    
    init_media_config(&config);

    file = "oss_media_file";
    write_file(file, &config);

    ret = oss_media_delete_file(&config, TEST_BUCKET_NAME, file);

    CuAssertIntEquals(tc, 0, ret);

    ret = oss_media_delete_file(&config, TEST_BUCKET_NAME, file);
    CuAssertIntEquals(tc, 0, ret);
}

void test_delete_file_failed_with_no_object(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    char *file = NULL;
    
    init_media_config(&config);

    file = ".not_exist_file";
    ret = oss_media_delete_file(&config, TEST_BUCKET_NAME, file);

    CuAssertIntEquals(tc, 0, ret);
}

void test_delete_file_failed_with_no_bucket(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    char *file = NULL;
    
    init_media_config(&config);

    file = "oss_media_file";
    ret = oss_media_delete_file(&config, "bucket-not-exist", file);

    CuAssertIntEquals(tc, -1, ret);
}

void test_list_files_succeeded(CuTest *tc) {
    int ret;    
    oss_media_config_t config;
    oss_media_files_t *file_list = NULL;
    char *file1 = NULL;
    char *file2 = NULL;
    
    init_media_config(&config);

    file1 = "oss_media_file.1";
    write_file(file1, &config);

    file2 = "oss_media_file.2";
    write_file(file2, &config);

    file_list = oss_media_create_files();

    ret = oss_media_list_files(&config, TEST_BUCKET_NAME, file_list);

    CuAssertIntEquals(tc, 0, ret);   

    ret = oss_media_delete_file(&config, TEST_BUCKET_NAME, file1);
    CuAssertIntEquals(tc, 0, ret);

    ret = oss_media_delete_file(&config, TEST_BUCKET_NAME, file2);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, NULL, file_list->path);
    CuAssertStrEquals(tc, NULL, file_list->marker);
    CuAssertIntEquals(tc, 0, file_list->max_size);
    CuAssertStrEquals(tc, NULL, file_list->next_marker);
    CuAssertIntEquals(tc, 2, file_list->size);
    CuAssertStrEquals(tc, file1, file_list->file_names[0]);
    CuAssertStrEquals(tc, file2, file_list->file_names[1]);

    oss_media_free_files(file_list);
}

void test_list_files_succeeded_with_no_file(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    oss_media_files_t *file_list = NULL;
    char *file1 = NULL;
    char *file2 = NULL;
    
    init_media_config(&config);

    file_list = oss_media_create_files();

    ret = oss_media_list_files(&config, TEST_BUCKET_NAME, file_list);

    CuAssertIntEquals(tc, 0, ret);

    CuAssertStrEquals(tc, NULL, file_list->path);
    CuAssertStrEquals(tc, NULL, file_list->marker);
    CuAssertIntEquals(tc, 0, file_list->max_size);
    CuAssertStrEquals(tc, NULL, file_list->next_marker);
    CuAssertIntEquals(tc, 0, file_list->size);

    oss_media_free_files(file_list);
}

void test_list_files_failed(CuTest *tc) {
    int ret;
    oss_media_config_t config;
    oss_media_files_t *file_list = NULL;
    char *file1 = NULL;
    char *file2 = NULL;
    
    init_media_config(&config);

    file_list = oss_media_create_files();

    ret = oss_media_list_files(&config, "bucket-not-exist", 
                              file_list);
    
    CuAssertIntEquals(tc, -1, ret);

    oss_media_free_files(file_list);
}

static int64_t write_file(const char* key, oss_media_config_t *config) {
    aos_pool_t *p = NULL;
    aos_string_t bucket;
    aos_string_t object;
    int is_oss_domain = 1;
    aos_table_t *headers = NULL;
    aos_table_t *resp_headers = NULL;
    oss_request_options_t *options = NULL;
    aos_list_t buffer;
    aos_buf_t *content = NULL;
    char *str = "test oss c sdk";
    aos_status_t *s = NULL;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    aos_str_set(&options->config->endpoint, config->endpoint);
    aos_str_set(&options->config->access_key_id, config->access_key_id);
    aos_str_set(&options->config->access_key_secret, config->access_key_secret);
    options->config->is_cname = config->is_cname;
    options->ctl = aos_http_controller_create(options->pool, 0);
    aos_str_set(&bucket, TEST_BUCKET_NAME);
    aos_str_set(&object, key);

    aos_list_init(&buffer);
    content = aos_buf_pack(options->pool, str, strlen(str));
    aos_list_add_tail(&content->node, &buffer);

    oss_put_object_from_buffer(options, &bucket, &object, 
                               &buffer, headers, &resp_headers);    
    
    aos_pool_destroy(p);
}

static void init_media_config(oss_media_config_t *config) {
    config->endpoint = TEST_OSS_ENDPOINT;
    config->access_key_id = TEST_ACCESS_KEY_ID;
    config->access_key_secret = TEST_ACCESS_KEY_SECRET;
    config->role_arn = TEST_ROLE_ARN; 
    config->is_cname = 0;
}

CuSuite *test_server()
{
    CuSuite* suite = CuSuiteNew();   

    SUITE_ADD_TEST(suite, test_server_setup);

    SUITE_ADD_TEST(suite, test_create_bucket_succeeded);
    SUITE_ADD_TEST(suite, test_create_bucket_failed);
    SUITE_ADD_TEST(suite, test_delete_bucket_succeeded);
    SUITE_ADD_TEST(suite, test_delete_bucket_failed);

    SUITE_ADD_TEST(suite, test_create_bucket_lifecycle_succeeded);
    SUITE_ADD_TEST(suite, test_create_bucket_lifecycle_failed);
    SUITE_ADD_TEST(suite, test_get_bucket_lifecycle_succeeded);
    SUITE_ADD_TEST(suite, test_get_bucket_lifecycle_failed);
    SUITE_ADD_TEST(suite, test_delete_bucket_lifecycle_succeeded);
    SUITE_ADD_TEST(suite, test_delete_bucket_lifecycle_failed);
    
    SUITE_ADD_TEST(suite, test_delete_file_succeeded);
    SUITE_ADD_TEST(suite, test_delete_file_failed_with_no_object);
    SUITE_ADD_TEST(suite, test_delete_file_failed_with_no_bucket);

    SUITE_ADD_TEST(suite, test_list_files_succeeded);
    SUITE_ADD_TEST(suite, test_list_files_succeeded_with_no_file);
    SUITE_ADD_TEST(suite, test_list_files_failed);
    
    SUITE_ADD_TEST(suite, test_server_teardown); 
    
    return suite;
}
