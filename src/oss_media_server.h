#ifndef OSS_MEDIA_SERVER_H
#define OSS_MEDIA_SERVER_H

#include "oss_c_sdk/aos_log.h"
#include "oss_c_sdk/aos_status.h"
#include "oss_c_sdk/aos_http_io.h"
#include "oss_c_sdk/oss_define.h"
#include "oss_c_sdk/oss_api.h"
#include "oss_media_define.h"

OSS_MEDIA_CPP_START

/**
 *  this struct describes the oss media configuration.
 */
typedef struct oss_media_config_s {
    char *endpoint;
    int8_t is_cname;
    char *access_key_id;
    char *access_key_secret;
    char *role_arn;
} oss_media_config_t;

/**
 * oss bucket acl
 * acl:
 *  OSS_ACL_PRIVATE: private write + private read
 *  OSS_ACL_PUBLIC_READ: private write + public  read
 *  OSS_ACL_PUBLIC_READ_WRITE: public write + public  read
 */
typedef oss_acl_e oss_media_acl_t;

/**
 *  @brief  oss bucket lifecycle rule. 
 *          oss file will be deleted automatic in specified days.
 */
typedef struct oss_media_lifecycle_rule_s {
    char *name;         // rule name
    char *path;         // oss media path
    char *status;       // Enabled or Disabled
    int   days;
} oss_media_lifecycle_rule_t;

/**
 *  @brief  oss bucket lifecycle rule sets. 
 *          this struct describes a collection of oss bucket lifecycle rules.
 */
typedef struct oss_media_lifecycle_rules_s {
    int size;                           // rule array size
    oss_media_lifecycle_rule_t **rules; // rule array
    aos_pool_t *_pool;
} oss_media_lifecycle_rules_t;


/**
 *  @bried  oss media files
 *          this struct describes a collection of oss media files.
 */
typedef struct oss_media_files_s {
    char *path;         // file path
    char *marker;       // paging identifier
    int  max_size;      // max items per page

    char *next_marker;  // paging identifier
    int  size;          // iterms per page
    char **file_names;  // file name array
} oss_media_files_t;

/**
 *  @brief  oss media token
 *          this struct describes the sts security token for accessing oss media file.
 */
typedef struct STSData oss_media_token_t;

/**
 *  @brief  oss media init
 *  @note    this function should be called at first in application
 */
int oss_media_init(aos_log_level_e log_level);

/*
 *  @brief  oss media destroy
 *  @note   this function should be called at last in application
 */
void oss_media_destroy();

/**
 *  @brief  create the data struct of oss_media_files_t
 */
oss_media_files_t* oss_media_create_files();

/**
 *  @brief  free the data struct of oss_media_files_t
 */
void oss_media_free_files(oss_media_files_t *files);

/**
 *  @brief  create the data struct of oss_media_lifecycle_rules_t
 */
oss_media_lifecycle_rules_t* oss_media_create_lifecycle_rules(int size);

/**
 *  @brief  free the data struct of oss_media_lifecycle_rules_t
 */
void oss_media_free_lifecycle_rules(oss_media_lifecycle_rules_t *rules);


/**
 *  @brief  create oss bucket. this function creates the oss bucket.
 *  @return:
 *      upon sucessful 0 is returned.
 *      otherwise, -1 is returned and code/message in data status is set to indicate the error.
 */
int oss_media_create_bucket(const oss_media_config_t *config, 
                            const char *bucket_name, 
                            oss_media_acl_t acl);

/*
 *  @brief  delete oss bucket. this function deletes the oss bucket.
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_delete_bucket(const oss_media_config_t *config, 
                            const char *bucket_name);

/**
 *  @brief  create bucket lifecycle. this function creates the bucket lifecycle.
 *  @note   under the specified rule, the oss file will be deleted in specified days.
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_create_bucket_lifecycle(const oss_media_config_t *config, 
                                      const char *bucket_name, 
                                      oss_media_lifecycle_rules_t *rules);

/**
 *  @brief  get bucket lifecycle. this function obtains the bucket lifecycle.
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_get_bucket_lifecycle(const oss_media_config_t *config, 
                                   const char *bucket_name, 
                                   oss_media_lifecycle_rules_t *rules);

/**
 *  @brief  delete bucket lifecycle. this function delete all lifecycles for the bucket.
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_delete_bucket_lifecycle(const oss_media_config_t *config, 
                                      const char *bucket_name);

/**
 *  @brief  delete oss file. this function delete the oss file.
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_delete_file(const oss_media_config_t *config, 
                          const char *bucket_name, 
                          const char *object_key);

/**
 *  @brief  list oss files. this function obtains the oss files.
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_list_files(const oss_media_config_t *config, 
                         const char *bucket_name, 
                         oss_media_files_t *files);

/*
 *  @brief  get sts token. this function obtains the access token.
 *  @param[in]:
      mode:
 *      'r': file access mode of read.
 *      'w': file access mode of write.
 *      'a': file access mode of append.
 *    expiration:
 *      access token expiration, in seconds, max value is 3600 (1 hour)
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_get_token(const oss_media_config_t *config, 
                        const char *bucket_name, 
                        const char *path, 
                        const char *mode, 
                        int64_t expiration,
                        oss_media_token_t *token);

/**
 *  @brief  get token from policy. this function obtains the access token, with sts policy.
 *  @param[in]:
 *      expiration:  access token expiration, in seconds, max value is 3600 (1 hour)
 *  @return:
 *      upon successful 0 is returned.
 *      otherwise, -1 is returned and code/message in struct status is set to indicate the error.
 */
int oss_media_get_token_from_policy(const oss_media_config_t *config, 
                                    const char *policy, 
                                    int64_t expiration,
                                    oss_media_token_t *token);

OSS_MEDIA_CPP_END
#endif
