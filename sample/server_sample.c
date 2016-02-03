#include <stdlib.h>
#include <stdio.h>
#include "src/oss_media_define.h"
#include "src/oss_media_server.h"
#include "src/sts/libsts.h"
#include "config.h"

static void init_media_config(oss_media_config_t *config) {
    config->endpoint = SAMPLE_OSS_ENDPOINT;
    config->access_key_id = SAMPLE_ACCESS_KEY_ID;
    config->access_key_secret = SAMPLE_ACCESS_KEY_SECRET;
    config->role_arn = SAMPLE_ROLE_ARN; 
    config->is_cname = 0;
}

void create_bucket() {
    int ret;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_create_bucket(&config, SAMPLE_BUCKET_NAME, OSS_ACL_PRIVATE);
    
    if (0 == ret) {
        printf("create bucket[%s] succeeded.\n", SAMPLE_BUCKET_NAME);
    } else {
        printf("create bucket[%s] failed.\n", SAMPLE_BUCKET_NAME);
    }
}

void delete_bucket() {
    int ret;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_delete_bucket(&config, SAMPLE_BUCKET_NAME);

    if (0 == ret) {
        printf("delete bucket[%s] succeeded.\n", SAMPLE_BUCKET_NAME);
    } else {
        printf("delete bucket[%s] failed.\n", SAMPLE_BUCKET_NAME);
    }    
}

void create_bucket_lifecycle() {
    int ret;
    oss_media_lifecycle_rules_t *rules;
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
            SAMPLE_BUCKET_NAME, rules);

    if (0 == ret) {
        printf("create bucket[%s] lifecycle succeeded.\n", SAMPLE_BUCKET_NAME);
    } else {
        printf("create bucket[%s] lifecycle failed.\n", SAMPLE_BUCKET_NAME);
    }

    oss_media_free_lifecycle_rules(rules);
}

void get_bucket_lifecycle() {
    int ret, i;
    oss_media_lifecycle_rules_t *rules;
    oss_media_config_t config;
    
    init_media_config(&config);

    rules = oss_media_create_lifecycle_rules(0);

    ret = oss_media_get_bucket_lifecycle(&config, SAMPLE_BUCKET_NAME, rules);

    if (0 == ret) {
        printf("get bucket[%s] lifecycle succeeded.\n", SAMPLE_BUCKET_NAME);
    } else {
        printf("get bucket[%s] lifecycle failed.\n", SAMPLE_BUCKET_NAME);
    }

    for (i = 0; i < rules->size; i++) {
        printf(">>>> rule: [name:%s, path:%s, status=%s, days=%d]\n",
               rules->rules[i]->name, rules->rules[i]->path, 
               rules->rules[i]->status, rules->rules[i]->days);
    }

    oss_media_free_lifecycle_rules(rules);
}

void delete_bucket_lifecycle() 
{
    int ret;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_delete_bucket_lifecycle(&config, SAMPLE_BUCKET_NAME);

    if (0 == ret) {
        printf("delete bucket[%s] lifecycle succeeded.\n", SAMPLE_BUCKET_NAME);
    } else {
        printf("delete bucket[%s] lifecycle failed.\n", SAMPLE_BUCKET_NAME);
    }
}

void delete_file() {
    int ret;
    oss_media_config_t config;
    char *file;
    
    init_media_config(&config);

    file = "oss_media_file";
    ret = oss_media_delete_file(&config, SAMPLE_BUCKET_NAME, file);

    if (0 == ret) {
        printf("delete file[%s] succeeded.\n", file);
    } else {
        printf("delete file[%s] lifecycle failed.\n", file);
    }
}

void list_files() {
    int ret, i;
    oss_media_files_t *files;
    oss_media_config_t config;
    
    init_media_config(&config);

    files = oss_media_create_files();
    files->max_size = 50;

    ret = oss_media_list_files(&config, SAMPLE_BUCKET_NAME, files);

    if (0 == ret) {
        printf("list files succeeded.\n");
    } else {
        printf("list files lifecycle failed.\n");
    }

    for (i = 0; i < files->size; i++) {
        printf(">>>>file name: %s\n", files->file_names[i]);
    }

    oss_media_free_files(files);
}

void get_token() {
    int ret;
    oss_media_token_t token;
    oss_media_config_t config;
    
    init_media_config(&config);

    ret = oss_media_get_token(&config, SAMPLE_BUCKET_NAME, "/*", "rwa", 
                            3600, &token);

    if (0 == ret) {
        printf("get token succeeded, access_key_id=%s, access_key_secret=%s, token=%s\n", 
               token.tmpAccessKeyId, token.tmpAccessKeySecret, token.securityToken);
    } else {
        printf("get token failed.\n");
    }
}

void get_token_from_policy() {
    int ret;
    oss_media_token_t token;
    char *policy;
    oss_media_config_t config;
    
    init_media_config(&config);

    policy = "{\"Version\":\"1\",\"Statement\":[{\"Effect\":\"Allow\", "
             "\"Action\":\"*\", \"Resource\":\"*\"}]}";

    ret = oss_media_get_token_from_policy(&config, policy, 3600, &token);

    if (0 == ret) {
        printf("get token succeeded, access_key_id=%s, access_key_secret=%s, token=%s\n", 
               token.tmpAccessKeyId, token.tmpAccessKeySecret, token.securityToken);
    } else {
        printf("get token failed.\n");
    }
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
    oss_media_init(AOS_LOG_INFO);
    
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
