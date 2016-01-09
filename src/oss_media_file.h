#ifndef OSS_MEDIA_CLIENT_H
#define OSS_MEDIA_CLIENT_H

#include "oss_c_sdk/aos_log.h"

#include "oss_c_sdk/aos_status.h"
#include "oss_c_sdk/aos_http_io.h"
#include "oss_c_sdk/oss_define.h"
#include "oss_c_sdk/oss_api.h"
#include "oss_media_define.h"

OSS_MEDIA_CPP_START

/**
 * this struct describes the file stat of oss media file
 */
typedef struct {
    int64_t length;    // file length, in bytes
    char    *type;     // file type, [Normal, Appendable]
} oss_media_file_stat_t;

/**
 *  this struct describes the properties of oss media file
 */
typedef struct oss_media_file_s {
    void   *ipc;              // ipc info

    char   *host;             // oss host
    int    is_oss_domain;     // oss host type, host is oss domain ? 1 : 0(cname etc)
    char   *bucket;           // oss bucket name
    char   *filename;         // oss object key
    char   *access_key_id;    // oss temporary access key
    char   *access_key_secret;// oss temporary access key secret
    char   *token;            // oss temporary access token
    time_t expiration;        // oss temporary access expiration

    void (* auth)(struct oss_media_file_s *file); // oss auth function

    char *mode;                 // open mode [r : read; w : write; a : append]
    char *_type;                // file type
    int64_t _length;            // file length
    int64_t _read_offset;       // read offset
} oss_media_file_t;

/**
 *  @brief  oss media init
 *  @note   this function should be called at first in application
 */
int oss_media_init();

/**
 *  @brief  oss media destroy
 *  @note   this function should be called at last in application
 */
void oss_media_destroy();

/**
 *  @brief  create the data struct of oss media file
 */
oss_media_file_t *oss_media_file_create();

/**
 *  @brief  free the data struct of oss media file
 */
void oss_media_file_free(oss_media_file_t *file);

/**
 *  @brief  open oss media file, this function opens the oss media file.
 *  @param[in]  param mode:
 *      'r': file access mode of read.
 *      'w': file access mode of write.
 *      'a': file access mode of append.
 *      notes: combined access mode is not allowd.
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error.
 *  
 */
int oss_media_file_open(oss_media_file_t *file, char *mode);

/**
 *  @brief  close oss media file
 */
int oss_media_file_close(oss_media_file_t *file);

/**
 *  @bref   stat file, this function obtains information about the file.
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise -1 is returned and code/message int struct of file is set to indicate the error.
 */
int oss_media_file_stat(oss_media_file_t *file, oss_media_file_stat_t *stat);

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
