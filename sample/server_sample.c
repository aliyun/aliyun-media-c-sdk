#include <stdlib.h>
#include <stdio.h>
#include "src/oss_media_define.h"
#include "src/oss_media.h"
#include "src/sts/libsts.h"
#include "config.h"

static void init_media_config(oss_media_config_t *config) {    

    config->host = SAMPLE_OSS_HOST;
    config->access_key_id = SAMPLE_ACCESS_KEY_ID;
    config->access_key_secret = SAMPLE_ACCESS_KEY_SECRET;
    config->role_arn = SAMPLE_ROLE_ARN; 
    config->is_oss_domain = 1;
}

void create_bucket() {
    int r;
    oss_media_status_t *status;
    oss_media_config_t config;
    
    init_media_config(&config);

    status = oss_media_create_status();
    r = oss_media_create_bucket(&config, SAMPLE_BUCKET_NAME, 
                                OSS_ACL_PRIVATE, status);
    
    printf("create bucket. [name=%s, ret=%d, http_code=%d, "
           "error_code=%s, error_message=%s]\n",
           SAMPLE_BUCKET_NAME, r, status->http_code, 
           status->error_code, status->error_msg);

    oss_media_free_status(status);
}

void delete_bucket() {
    int r;
    oss_media_status_t *status;
    oss_media_config_t config;
    
    init_media_config(&config);
    
    status = oss_media_create_status();

    r = oss_media_delete_bucket(&config, SAMPLE_BUCKET_NAME, status);

    printf("delete bucket. [name=%s, ret=%d, http_code=%d, error_code=%s, "
           "error_message=%s]\n", SAMPLE_BUCKET_NAME, r, status->http_code, 
           status->error_code, status->error_msg);
    
    oss_media_free_status(status);
}

void create_bucket_lifecycle() {
    int r;
    oss_media_status_t *status;
    oss_media_lifecycle_rules_t *rules;
    oss_media_config_t config;
    
    init_media_config(&config);

    status = oss_media_create_status();
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

    r = oss_media_create_bucket_lifecycle(&config, 
            SAMPLE_BUCKET_NAME, rules, status);

    printf("create bucket lifecycle. [name=%s, ret=%d, http_code=%d, "
           "error_code=%s, error_message=%s]\n", SAMPLE_BUCKET_NAME, r, 
           status->http_code, status->error_code, status->error_msg); 

    oss_media_free_lifecycle_rules(rules);
    oss_media_free_status(status);
}

void get_bucket_lifecycle() {
    int r, i;
    oss_media_status_t *status;
    oss_media_lifecycle_rules_t *rules;
    oss_media_config_t config;
    
    init_media_config(&config);

    status = oss_media_create_status();
    rules = oss_media_create_lifecycle_rules(0);

    r = oss_media_get_bucket_lifecycle(&config, SAMPLE_BUCKET_NAME, rules, status);
    printf("get bucket lifecycle. [name=%s, ret=%d, http_code=%d, "
           "error_code=%s, error_message=%s]\n", SAMPLE_BUCKET_NAME, r, 
           status->http_code, status->error_code, status->error_msg);

    for (i=0; i<rules->size; i++) {
        printf(">>>> rule: [name:%s, path:%s, status=%s, days=%d]\n",
               rules->rules[i]->name, rules->rules[i]->path, 
               rules->rules[i]->status, rules->rules[i]->days);
    }

    oss_media_free_lifecycle_rules(rules);
    oss_media_free_status(status);
}

void delete_bucket_lifecycle() 
{
    int r;
    oss_media_status_t *status;
    oss_media_config_t config;
    
    init_media_config(&config);
    status = oss_media_create_status();

    r = oss_media_delete_bucket_lifecycle(&config, SAMPLE_BUCKET_NAME, status);

    printf("delete bucket lifecycle. [name=%s, ret=%d, http_code=%d, "
           "error_code=%s, error_message=%s]\n", SAMPLE_BUCKET_NAME, r, 
           status->http_code, status->error_code, status->error_msg);

    oss_media_free_status(status);
}

void delete_file() {
    int r;
    oss_media_status_t *status;
    oss_media_config_t config;
    char *file;
    
    init_media_config(&config);
    status = oss_media_create_status();

    file = "oss_media_file";
    r = oss_media_delete_file(&config, SAMPLE_BUCKET_NAME, file, status);

    printf("delete file. [name=%s, file=%s, ret=%d, http_code=%d, "
           "error_code=%s, error_message=%s]\n",
           SAMPLE_BUCKET_NAME, file, r, status->http_code, 
           status->error_code, status->error_msg);

    oss_media_free_status(status);
}

void list_files() {
    int i, r;
    oss_media_status_t *status;
    oss_media_files_t *files;
    oss_media_config_t config;
    
    init_media_config(&config);
    status = oss_media_create_status();

    files = oss_media_create_files();
    files->max_size = 50;

    r = oss_media_list_files(&config, SAMPLE_BUCKET_NAME, files, status);

    printf("list files. [name=%s, ret=%d, http_code=%d, error_code=%s, "
           "error_message=%s]\n", SAMPLE_BUCKET_NAME, r, status->http_code, 
           status->error_code, status->error_msg);

    for (i = 0; i < files->size; i++) {
        printf(">>>>file name: %s\n", files->file_names[i]);
    }

    oss_media_free_files(files);
    oss_media_free_status(status);
}

void get_token() {
    int r;
    oss_media_status_t *status;
    oss_media_token_t  token;
    oss_media_config_t config;
    
    init_media_config(&config);
    status = oss_media_create_status();

    r = oss_media_get_token(&config, SAMPLE_BUCKET_NAME, "/*", "rwa", 
                            3600, &token, status);

    printf("get token. [access_key_id=%s, access_key_secret=%s, token=%s, "
           "ret=%d, http_code=%d, error_code=%s, error_message=%s]\n",
           token.tmpAccessKeyId, token.tmpAccessKeySecret, token.securityToken, 
           r, status->http_code, status->error_code, status->error_msg);

    oss_media_free_status(status);
}

void get_token_from_policy() {
    int r;
    oss_media_status_t *status;
    oss_media_token_t  *token;
    char *policy;
    oss_media_config_t config;
    
    init_media_config(&config);

    policy = "{\"Version\":\"1\",\"Statement\":[{\"Effect\":\"Allow\", "
             "\"Action\":\"*\", \"Resource\":\"*\"}]}";
    
    token = (STSData*) malloc(sizeof(STSData));
    status = oss_media_create_status();

    r = oss_media_get_token_from_policy(&config, policy, 3600, token, status);

    printf("get token from policy succeed. [access_key_id=%s, "
           "access_key_secret=%s, token=%s, ret=%d, http_code=%d, "
           "error_code=%s, error_message=%s]\n",token->tmpAccessKeyId, 
           token->tmpAccessKeySecret, token->securityToken, r,
           status->http_code, status->error_code, status->error_msg);

    free(token);
    oss_media_free_status(status);
}

static int usage() {
    printf("Usage: oss_media_server_example type\n"
           "type:\n"
           "     create_bucket\n"
           "     delete_bucket\n"
           "     delete_file\n"
           "     list_files\n"
           "     create_bucket_lifecycle\n"
           "     get_bucket_lifecycle\n"
           "     delete_bucket_lifecycle\n"
           "     get_token\n"
           "     get_token_from_policy\n");
    return -1;
}

int main(int argc, char *argv[]) {
    oss_media_init();
    
    if (argc < 2) {
        usage();
        oss_media_destroy();
        return -1;
    }

    if (strcmp("create_bucket", argv[1]) == 0) {
        create_bucket();
    }
    else if (strcmp("delete_bucket", argv[1]) == 0) {
        delete_bucket();
    }
    else if (strcmp("delete_file", argv[1]) == 0) {
        delete_file();
    }
    else if (strcmp("list_files", argv[1]) == 0) {
        list_files();
    }
    else if (strcmp("create_bucket_lifecycle", argv[1]) == 0) {
        create_bucket_lifecycle();
    }
    else if (strcmp("get_bucket_lifecycle", argv[1]) == 0) {
        get_bucket_lifecycle();
    }
    else if (strcmp("delete_bucket_lifecycle", argv[1]) == 0) {
        delete_bucket_lifecycle();
    }
    else if (strcmp("get_token", argv[1]) == 0) {
        get_token();
    }
    else if (strcmp("get_token_from_policy", argv[1]) == 0) {
        get_token_from_policy();
    } 
    else {
        printf("Unsupport operation: %s\n", argv[1]);
        usage();
    }

    oss_media_destroy();
    
    return 0;
}
