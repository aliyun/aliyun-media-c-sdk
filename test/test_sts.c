#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_server.h"
#include "src/oss_media_client.h"
#include "src/sts/libsts.h"
#include <oss_c_sdk/aos_define.h>
#include <oss_c_sdk/aos_status.h>

static void init_media_config(oss_media_config_t *config);
static int64_t write_file(const char* key, oss_media_config_t *config);
static void* delete_file(oss_media_file_t *file);
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
        oss_media_token_t token;
        oss_media_config_t config;
    
        init_media_config(&config);

        ret = oss_media_get_token(&config, TEST_BUCKET_NAME, "/*", "rwa",
                15 * 60, &token);

        CuAssertIntEquals(tc, 0, ret);

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

        // open file
        file = oss_media_file_open("w", auth_func);
        CuAssertTrue(tc, NULL != file);

        // write file
        write_size = oss_media_file_write(file, content, strlen(content));
        CuAssertIntEquals(tc, strlen(content), write_size);

        ret = oss_media_file_stat(file, &stat);
        CuAssertIntEquals(tc, 0, ret);

        CuAssertStrEquals(tc, "Normal", stat.type);
        CuAssertIntEquals(tc, write_size, stat.length);
        
        delete_file(file);

        // close file
        clear_client_token();
        oss_media_file_close(file);
    }
}

void test_get_token_failed_with_expiration_less_than_15m(CuTest *tc) {
    int ret;
    oss_media_token_t  token;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_get_token(&config, TEST_BUCKET_NAME, "/*", "rwa",
                              15 * 60 - 1, &token);

    CuAssertIntEquals(tc, -1, ret);
}

void test_get_token_failed_with_expiration_more_than_1h(CuTest *tc) {
    int ret;
    oss_media_token_t  token;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_get_token(&config, TEST_BUCKET_NAME, "/*", "rwa",
                              60 * 60 + 1, &token);

    CuAssertIntEquals(tc, -1, ret);
}

void test_get_and_use_token_from_policy_succeeded(CuTest *tc) {
    oss_media_token_t  *token = NULL;
    // server get token and send to client
    {
        int ret;
        char *policy = NULL;
        oss_media_config_t config;
    
        policy = "{\n"
                 "\"Statement\": [\n"
                 "{"
                 "\"Action\": \"oss:*\",\n"
                 "\"Effect\": \"Allow\",\n"
                 "\"Resource\": \"*\"\n"
                 "}\n"
                 "],\n"
                 "\"Version\": \"1\"\n"
                 "}\n";

        init_media_config(&config);

        token = (STSData*) malloc(sizeof(STSData));

        ret = oss_media_get_token_from_policy(&config, policy,
                15 * 60, token);

        CuAssertIntEquals(tc, 0, ret);

        send_token_to_client(*token);
    }

    // client operation
    {
        int ret;
        int64_t write_size = 0;
        oss_media_file_t *file = NULL;
        char *content = NULL;
        oss_media_file_stat_t stat;

        content = "hello oss media file\n";

        // open file
        file = oss_media_file_open("w", auth_func);
        CuAssertTrue(tc, NULL != file);

        // write file
        write_size = oss_media_file_write(file, content, strlen(content));
        CuAssertIntEquals(tc, strlen(content), write_size);

        ret = oss_media_file_stat(file, &stat);
        CuAssertIntEquals(tc, 0, ret);

        CuAssertStrEquals(tc, "Normal", stat.type);
        CuAssertIntEquals(tc, write_size, stat.length);
        
        delete_file(file);

        // close file
        clear_client_token();
        free(token);
        oss_media_file_close(file);
    }
}

void test_get_and_use_token_from_policy_failed_with_policy_disallow(CuTest *tc) {
    oss_media_token_t  *token = NULL;
    // server get token and send to client
    {
        int ret;
        char *policy = NULL;
        oss_media_config_t config;
    
        policy = "{\n"
                 "\"Statement\": [\n"
                 "{"
                 "\"Action\": \"oss:GetObject\",\n"
                 "\"Effect\": \"Allow\",\n"
                 "\"Resource\": \"*\"\n"
                 "}\n"
                 "],\n"
                 "\"Version\": \"1\"\n"
                 "}\n";
        
        init_media_config(&config);
        
        token = (STSData*) malloc(sizeof(STSData));

        ret = oss_media_get_token_from_policy(&config, policy,
                15 * 60, token);

        CuAssertIntEquals(tc, 0, ret);

        send_token_to_client(*token);
    }

    // client operation
    {
        int ret;
        int64_t write_size = 0;
        oss_media_file_t *file = NULL;
        char *content = NULL;
        oss_media_file_stat_t stat;

        content = "hello oss media file\n";

        // open file
        file = oss_media_file_open("w", auth_func);
        CuAssertTrue(tc, NULL != file);

        //write file
        write_size = oss_media_file_write(file, content, strlen(content));
        CuAssertIntEquals(tc, -1, write_size);

        sleep(1);
        ret = oss_media_file_stat(file, &stat);
        CuAssertIntEquals(tc, 0, ret);

        CuAssertStrEquals(tc, "unknown", stat.type);
        CuAssertIntEquals(tc, 0, stat.length);
        
        delete_file(file);

        // close file and free
        clear_client_token();
        free(token);
        oss_media_file_close(file);
    }
}

static void auth_func(oss_media_file_t *file) {
    file->endpoint = TEST_OSS_ENDPOINT;
    file->is_cname = 0;
    file->bucket_name = TEST_BUCKET_NAME;
    file->object_key = "oss_media_file";
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
    int is_cname = 0;
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
    aos_str_set(&options->config->endpoint, config->endpoint);
    aos_str_set(&options->config->access_key_id, config->access_key_id);
    aos_str_set(&options->config->access_key_secret, config->access_key_secret);
    options->config->is_cname = config->is_cname;
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
    config->endpoint = TEST_OSS_ENDPOINT;
    config->access_key_id = TEST_ACCESS_KEY_ID;
    config->access_key_secret = TEST_ACCESS_KEY_SECRET;
    config->role_arn = TEST_ROLE_ARN; 
    config->is_cname = 0;
}

static void* delete_file(oss_media_file_t *file) {
    aos_pool_t *p;
    aos_string_t bucket;
    aos_string_t object;
    int is_oss_domain = 1;
    oss_request_options_t *options;
    aos_table_t *resp_headers;

    aos_pool_create(&p, NULL);
    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    aos_str_set(&options->config->endpoint, file->endpoint);
    aos_str_set(&options->config->access_key_id, TEST_ACCESS_KEY_ID);
    aos_str_set(&options->config->access_key_secret, TEST_ACCESS_KEY_SECRET);
    options->config->is_cname = file->is_cname;
    options->ctl = aos_http_controller_create(options->pool, 0);    
    aos_str_set(&bucket, file->bucket_name);
    aos_str_set(&object, file->object_key);

    oss_delete_object(options, &bucket, &object, &resp_headers);
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
    SUITE_ADD_TEST(suite, test_get_and_use_token_from_policy_succeeded);
    SUITE_ADD_TEST(suite, test_get_and_use_token_from_policy_failed_with_policy_disallow);
    
    return suite;
}
