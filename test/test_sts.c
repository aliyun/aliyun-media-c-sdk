#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media.h"
#include "src/oss_media_file.h"
#include "src/sts/libsts.h"
#include <oss_c_sdk/aos_define.h>
#include <oss_c_sdk/aos_status.h>

static void init_media_config(oss_media_config_t *config);
static int64_t write_file(const char* key, oss_media_config_t *config);
static aos_status_t* delete_file(oss_media_file_t *file);
static void send_token_to_client(oss_media_token_t token);
static void clear_client_token();
static void auth_func(oss_media_file_t *file);

char* global_temp_access_key_id = NULL;
char* global_temp_access_key_secret = NULL;
char* global_temp_token = NULL;

void test_get_and_use_token_succeeded(CuTest *tc) {
    // server get token and send to client
    {
        int ret;
        oss_media_status_t *status;
        oss_media_token_t token;
        oss_media_config_t config;
    
        init_media_config(&config);
        status = oss_media_create_status();

        ret = oss_media_get_token(&config, TEST_BUCKET_NAME, "/*", "rwa",
                15 * 60, &token, status);

        CuAssertIntEquals(tc, 0, ret);
        CuAssertIntEquals(tc, 200, status->http_code);
        CuAssertStrEquals(tc, "", status->error_code);
        CuAssertStrEquals(tc, "", status->error_msg);

        oss_media_free_status(status);

        send_token_to_client(token);
    }

    // client operation
    {
        int ret;
        int64_t write_size = 0;
        oss_media_file_t *file = NULL;
        char *content = NULL;
        oss_media_file_stat_t stat;

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

        ret = oss_media_file_stat(file, &stat);
        CuAssertIntEquals(tc, 0, ret);

        CuAssertStrEquals(tc, "Normal", stat.type);
        CuAssertIntEquals(tc, write_size, stat.length);
        
        delete_file(file);

        // close file and free
        clear_client_token();
        oss_media_file_close(file);
        oss_media_file_free(file);
    }
}

void test_get_token_failed_with_expiration_less_than_15m(CuTest *tc) {
    int ret;
    oss_media_status_t *status;
    oss_media_token_t  token;
    oss_media_config_t config;
    
    init_media_config(&config);
    status = oss_media_create_status();

    ret = oss_media_get_token(&config, TEST_BUCKET_NAME, "/*", "rwa",
                              15 * 60 - 1, &token, status);

    CuAssertIntEquals(tc, -1, ret);
    CuAssertIntEquals(tc, 500, status->http_code);
    CuAssertStrEquals(tc, "GetSTSTokenError", status->error_code);

    oss_media_free_status(status);
}

void test_get_token_failed_with_expiration_more_than_1h(CuTest *tc) {
    int ret;
    oss_media_status_t *status;
    oss_media_token_t  token;
    oss_media_config_t config;
    
    init_media_config(&config);
    status = oss_media_create_status();

    ret = oss_media_get_token(&config, TEST_BUCKET_NAME, "/*", "rwa",
                              60 * 60 + 1, &token, status);

    CuAssertIntEquals(tc, -1, ret);
    CuAssertIntEquals(tc, 500, status->http_code);
    CuAssertStrEquals(tc, "GetSTSTokenError", status->error_code);

    oss_media_free_status(status);
}

static void auth_func(oss_media_file_t *file) {
    file->host = TEST_OSS_HOST;
    file->is_oss_domain = 1; //oss host type, host is oss domain ? 1 : 0(cname etc)
    file->bucket = TEST_BUCKET_NAME;
    file->filename = "oss_media_file";
    file->access_key_id = global_temp_access_key_id;
    file->access_key_secret = global_temp_access_key_secret;
    file->token = global_temp_token; 

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

static int64_t write_file(const char* key, oss_media_config_t *config) {
    aos_pool_t *p;
    aos_string_t bucket;
    aos_string_t object;
    int is_oss_domain = 1;
    aos_table_t *headers;
    aos_table_t *resp_headers;
    oss_request_options_t *options;
    aos_list_t buffer;
    aos_buf_t *content;
    char *str = "test oss c sdk";
    aos_status_t *s;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    aos_str_set(&options->config->host, config->host);
    aos_str_set(&options->config->id, config->access_key_id);
    aos_str_set(&options->config->key, config->access_key_secret);
    options->config->is_oss_domain = config->is_oss_domain;
    options->ctl = aos_http_controller_create(options->pool, 0);    
    headers = aos_table_make(p, 0);
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
    config->host = TEST_OSS_HOST;
    config->access_key_id = TEST_ACCESS_KEY_ID;
    config->access_key_secret = TEST_ACCESS_KEY_SECRET;
    config->role_arn = TEST_ROLE_ARN; 
    config->is_oss_domain = 1;
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
    aos_str_set(&options->config->id, TEST_ACCESS_KEY_ID);
    aos_str_set(&options->config->key, TEST_ACCESS_KEY_SECRET);
    options->config->is_oss_domain = file->is_oss_domain;
    options->ctl = aos_http_controller_create(options->pool, 0);    
    aos_str_set(&bucket, file->bucket);
    aos_str_set(&object, file->filename);

    return oss_delete_object(options, &bucket, &object, &resp_headers);
}

static void send_token_to_client(oss_media_token_t token) {
    global_temp_access_key_id = token.tmpAccessKeyId;
    global_temp_access_key_secret = token.tmpAccessKeySecret;
    global_temp_token = token.securityToken;
}

static void clear_client_token() {
    global_temp_access_key_id = NULL;
    global_temp_access_key_secret = NULL;
    global_temp_token = NULL;
}

CuSuite *test_sts()
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_get_and_use_token_succeeded);
    SUITE_ADD_TEST(suite, test_get_token_failed_with_expiration_less_than_15m);
    SUITE_ADD_TEST(suite, test_get_token_failed_with_expiration_more_than_1h);

    return suite;
}
