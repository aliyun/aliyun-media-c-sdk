#ifndef OSS_MEDIA_CLIENT_H
#define OSS_MEDIA_CLIENT_H

#include <oss_c_sdk/aos_log.h>
#include <oss_c_sdk/aos_status.h>
#include <oss_c_sdk/aos_http_io.h>
#include <oss_c_sdk/oss_define.h>
#include <oss_c_sdk/oss_api.h>
#include "oss_media_define.h"

OSS_MEDIA_CPP_START

/**
 * this struct describes the file stat of oss media file
 */
typedef struct {
    int64_t length;
    int64_t pos;
    char    *type;
} oss_media_file_stat_t;

/**
 *  this typedef define the auth_fn_t.
 */
struct oss_media_file_s;
typedef void (*auth_fn_t)(struct oss_media_file_s *file);

/**
 *  this struct describes the properties of oss media file
 */
typedef struct oss_media_file_s {
    void   *ipc;
    char   *endpoint;
    int8_t is_cname; 
    char   *bucket_name;  
    char   *object_key;
    char   *access_key_id;
    char   *access_key_secret;
    char   *token;
    char   *mode;
    oss_media_file_stat_t _stat;

    time_t expiration;
    auth_fn_t auth_func;
} oss_media_file_t;

/**
 *  @brief  oss media init
 *  @note   this function should be called at first in application
 */
int oss_media_init(aos_log_level_e log_level);

/**
 *  @brief  oss media destroy
 *  @note   this function should be called at last in application
 */
void oss_media_destroy();

/**
 *  @brief  open oss media file, this function opens the oss media file.
 *  @param[in]  bucket_name the bucket name for store file in oss
 *  @param[in]  object_key the object name for oss file
 *  @param[in]  mode:
 *      'r': file access mode of read.
 *      'w': file access mode of write.
 *      'a': file access mode of append.
 *      notes: combined access mode is not allowd.
 *  @param[in]  auth_func the func to set access_key_id/access_key_secret
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error. 
 */
oss_media_file_t* oss_media_file_open(char *bucket_name,
                                      char *object_key,
                                      char *mode,
                                      auth_fn_t auth_func);

/**
 *  @brief  close oss media file
 */
void oss_media_file_close(oss_media_file_t *file);

/**
 *  @bref   stat file, this function obtains information about the file.
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int oss_media_file_stat(oss_media_file_t *file, oss_media_file_stat_t *stat);

/**
 *  @bref   delete file, this function delete the oss media file.
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int oss_media_file_delete(oss_media_file_t *file);


/**
 *  @brief  tell the position of the oss media file
 *  @return:
 *      upon successful return the position of the oss media file.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int64_t oss_media_file_tell(oss_media_file_t *file);

/**
 *  @brief  set the position of the oss media file
 *  @return:
 *      upon successful return the postition of the oss media file.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int64_t oss_media_file_seek(oss_media_file_t *file, int64_t offset);

/**
 *  @brief  read from oss media file, this function reads number of bytes from the oss media file.
 *  @note   buf size should longer than nbyte + 1
 *  @return:
 *      upon successful return the number of bytes read.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int64_t oss_media_file_read(oss_media_file_t *file, void *buf, int64_t nbyte);

/**
 *  @brief  write to oss media file, this function write number of bytes to oss media file.
 *  @return:
 *      upon successful return the number of bytes write.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int64_t oss_media_file_write(oss_media_file_t *file, const void *buf, int64_t nbyte);

OSS_MEDIA_CPP_END

#endif 
