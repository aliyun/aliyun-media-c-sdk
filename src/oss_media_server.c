#include "oss_media_server.h"
#include "sts/libsts.h"

extern void oss_op_debug(char *op, 
                         aos_status_t *status, 
                         aos_table_t *req_headers, 
                         aos_table_t *resp_headers);

static void oss_media_init_request_opts(aos_pool_t *pool, 
                                        const oss_media_config_t *config, 
                                        oss_request_options_t **options) 
{
    oss_request_options_t *opts = NULL;

    opts = oss_request_options_create(pool);
    opts->config = oss_config_create(pool);
    aos_str_set(&opts->config->endpoint, config->endpoint);
    aos_str_set(&opts->config->access_key_id, config->access_key_id);
    aos_str_set(&opts->config->access_key_secret, config->access_key_secret);
    opts->config->is_cname = config->is_cname;
    opts->ctl = aos_http_controller_create(pool, 0);

    *options = opts;
}

int oss_media_init(aos_log_level_e log_level) 
{
    aos_log_set_level(log_level);
    aos_log_set_output(NULL);
    return aos_http_io_initialize(OSS_MEDIA_SERVER_USER_AGENT, 0);
}

void oss_media_destroy() 
{
    aos_http_io_deinitialize();
}

oss_media_files_t* oss_media_create_files() 
{
    return (oss_media_files_t *) calloc(sizeof(oss_media_files_t), 1);
}

void oss_media_free_files(oss_media_files_t *files) 
{
    int i;

    if (!files)
        return;

    if (files->file_names) {
        for (i = 0 ; i < files->size; i++) {
            if (files->file_names[i]) {
                free(files->file_names[i]);
                files->file_names[i] = NULL;
            }
        }
        free(files->file_names);
        files->file_names = NULL;
    }
    free(files);
    files = NULL;
}

oss_media_lifecycle_rules_t *oss_media_create_lifecycle_rules(int size)
{
    aos_pool_t *pool = NULL;
    oss_media_lifecycle_rules_t *rules = NULL;

    aos_pool_create(&pool, NULL);

    rules = (oss_media_lifecycle_rules_t *) aos_pcalloc(
            pool, sizeof(oss_media_lifecycle_rules_t));

    rules->size  = size > 0 ? size : 0;
    rules->rules = size > 0 ? aos_pcalloc(pool, 
            sizeof(oss_media_lifecycle_rule_t) * rules->size) : NULL;

    rules->_pool = pool;
    return rules;
}

void oss_media_free_lifecycle_rules(oss_media_lifecycle_rules_t *rules) 
{
    aos_pool_destroy(rules->_pool);
}

int oss_media_create_bucket(const oss_media_config_t *config,
                            const char *bucket_name,
                            oss_media_acl_t acl)
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_status_t *status = NULL;
    aos_table_t *resp_headers = NULL;

    aos_pool_create(&pool, NULL);
    oss_media_init_request_opts(pool, config, &opts);
    aos_str_set(&bucket, bucket_name);

    status = oss_create_bucket(opts, &bucket, acl, &resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("create bucket failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    aos_pool_destroy(pool);
    return 0;
}

int oss_media_delete_bucket(const oss_media_config_t *config, 
                            const char *bucket_name) 
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *options = NULL;
    aos_string_t bucket;
    aos_status_t *status = NULL;
    aos_table_t *resp_headers = NULL;

    aos_pool_create(&pool, NULL);
    options = oss_request_options_create(pool);
    oss_media_init_request_opts(pool, config, &options);
    aos_str_set(&bucket, bucket_name);

    status = oss_delete_bucket(options, &bucket, &resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("delete bucket failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    aos_pool_destroy(pool);
    return 0;
}

int oss_media_create_bucket_lifecycle(const oss_media_config_t *config, 
                                      const char *bucket_name, 
                                      oss_media_lifecycle_rules_t *rules) 
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_status_t *status = NULL;
    aos_table_t *resp_headers = NULL;
    aos_list_t rule_list;
    oss_lifecycle_rule_content_t *rule_content = NULL;
    int i;

    aos_pool_create(&pool, NULL);
    opts = oss_request_options_create(pool);
    oss_media_init_request_opts(pool, config, &opts);
    aos_str_set(&bucket, bucket_name);

    aos_list_init(&rule_list);
    for (i = 0; i < rules->size; i++) {
        rule_content = oss_create_lifecycle_rule_content(pool);
        aos_str_set(&rule_content->id, rules->rules[i]->name);
        aos_str_set(&rule_content->prefix, rules->rules[i]->path);
        aos_str_set(&rule_content->status, rules->rules[i]->status);
        rule_content->days = rules->rules[i]->days;

        aos_list_add_tail(&rule_content->node, &rule_list);
    }

    status = oss_put_bucket_lifecycle(opts, &bucket, &rule_list, &resp_headers);

    oss_op_debug("oss_put_bucket_lifecycle", status, NULL, resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("create bucket lifecycle failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    aos_pool_destroy(pool);
    return 0;
}

int oss_media_get_bucket_lifecycle(const oss_media_config_t *config, 
                                   const char *bucket_name, 
                                   oss_media_lifecycle_rules_t *rules) 
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_status_t *status = NULL;
    aos_table_t *resp_headers = NULL;
    aos_list_t rule_list;
    oss_lifecycle_rule_content_t *rule_content = NULL;
    int i;

    aos_pool_create(&pool, NULL);
    opts = oss_request_options_create(pool);
    oss_media_init_request_opts(pool, config, &opts);
    aos_str_set(&bucket, bucket_name);
    aos_list_init(&rule_list);
    
    status = oss_get_bucket_lifecycle(opts, &bucket, 
            &rule_list, &resp_headers);

    oss_op_debug("oss_get_bucket_lifecycle", status, NULL, resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("get bucket lifecycle failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    i = 0;
    aos_list_for_each_entry(rule_content, &rule_list, node) {
        i++;
    }
    rules->size = i;
    rules->rules = aos_pcalloc(rules->_pool, 
                               sizeof(oss_media_lifecycle_rule_t) * rules->size);

    i = 0;
    aos_list_for_each_entry(rule_content, &rule_list, node) {
        oss_media_lifecycle_rule_t *rule = 
            aos_pcalloc(rules->_pool, sizeof(oss_media_lifecycle_rule_t));
        rule->name = aos_pstrdup(rules->_pool, &rule_content->id);
        rule->path = aos_pstrdup(rules->_pool, &rule_content->prefix);
        rule->status = aos_pstrdup(rules->_pool, &rule_content->status);
        rule->days = rule_content->days;

        rules->rules[i++] = rule;
    }

    aos_pool_destroy(pool);
    return 0;
}

int oss_media_delete_bucket_lifecycle(const oss_media_config_t *config, 
                                      const char *bucket_name)
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_status_t *status = NULL;
    aos_table_t *resp_headers = NULL;

    aos_pool_create(&pool, NULL);
    opts = oss_request_options_create(pool);
    oss_media_init_request_opts(pool, config, &opts);
    aos_str_set(&bucket, bucket_name);

    status = oss_delete_bucket_lifecycle(opts, &bucket, &resp_headers);

    oss_op_debug("oss_delete_bucket_lifecycle", status, NULL, resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("delete bucket lifecycle failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }
          
    aos_pool_destroy(pool); 
    return 0;
}

int oss_media_delete_file(const oss_media_config_t *config, 
                          const char *bucket_name, 
                          const char *key)
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_string_t object;
    aos_status_t *status = NULL;
    aos_table_t *resp_headers = NULL;

    aos_pool_create(&pool, NULL);
    opts = oss_request_options_create(pool);
    oss_media_init_request_opts(pool, config, &opts);
    aos_str_set(&bucket, bucket_name);
    aos_str_set(&object, key);

    status = oss_delete_object(opts, &bucket, &object, &resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("delete file failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    aos_pool_destroy(pool);
    return 0;
}

int oss_media_list_files(const oss_media_config_t *config, 
                         const char *bucket_name, 
                         oss_media_files_t *files) 
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_status_t *status = NULL;
    oss_list_object_params_t *params = NULL;
    aos_table_t *resp_headers = NULL;
    int i;
    aos_list_t *head = NULL;
    oss_list_object_content_t *content = NULL;

    aos_pool_create(&pool, NULL);
    opts = oss_request_options_create(pool);
    oss_media_init_request_opts(pool, config, &opts);
    aos_str_set(&bucket, bucket_name);

    params = oss_create_list_object_params(pool);
    params->max_ret = files->max_size > 0 && 
                      files->max_size <= OSS_MEDIA_FILE_NUM_PER_LIST ? 
                      files->max_size : OSS_MEDIA_FILE_NUM_PER_LIST;
    if (files->path)
        aos_str_set(&params->prefix, files->path);
    if (files->marker)
        aos_str_set(&params->marker, files->marker);

    status = oss_list_object(opts, &bucket, params, &resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("list files failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    head = &params->object_list;
    aos_list_for_each_entry(content, head, node) {
        files->size++;
    }
    files->file_names = calloc(sizeof(char *), files->size);

    i = 0;
    aos_list_for_each_entry(content, head, node) {
        files->file_names[i++] = strdup(content->key.data);
    }

    aos_pool_destroy(pool);
    return 0;
}

int oss_media_get_token(const oss_media_config_t *config, 
                        const char *bucket_name, 
                        const char *path, 
                        const char *mode, 
                        int64_t expiration, 
                        oss_media_token_t *token) 
{
    char policy[1024];
    char *policy_template = NULL;
    char *h = NULL;
    char *r = NULL;
    char *w = NULL;
    char *a = NULL;

    policy_template = "{\"Version\":\"1\",\"Statement\":[{\"Effect\":\"Allow\","
                      " \"Action\":[\"%s\", \"%s\", \"%s\", \"%s\"], "
                      "\"Resource\":\"acs:oss:*:*:%s%s\"}]}";

    h = "oss:HeadObject";
    r = (strchr(mode, 'r') != NULL) ? "oss:GetObject" : "";
    w = (strchr(mode, 'w') != NULL) ? "oss:PutObject" : "";
    a = (strchr(mode, 'a') != NULL) ? "oss:PutObject" : "";

    sprintf(policy, policy_template, h, r, w, a, bucket_name, path);

    return oss_media_get_token_from_policy(config, 
            policy, expiration, token);
}

int oss_media_get_token_from_policy(const oss_media_config_t *config, 
                                    const char *policy, 
                                    int64_t expiration,
                                    oss_media_token_t *token) 
{
    char errorMessage[1024];
    int sts_status;

    sts_status = STS_assume_role(config->role_arn, OSS_MEDIA_SERVER_USER_AGENT,
                                 policy, expiration, config->access_key_id,
                                 config->access_key_secret, token, errorMessage);
    
    if (STSStatusOK != sts_status) {
        aos_error_log("get token failed. [role_arn:%s, policy:%s, status:%d, "
                      "error message:%s]", config->role_arn, policy, sts_status,
                      errorMessage);
        return -1;
    }
    
    return 0;
}
