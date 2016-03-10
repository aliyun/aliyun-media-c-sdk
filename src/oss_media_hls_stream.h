#ifndef OSS_MEDIA_HLS_STREAM_H
#define OSS_MEDIA_HLS_STREAM_H

#include "oss_media_hls.h"

/**
 * this struct describes the properties of hls stream options
 */
typedef struct oss_media_hls_stream_options_s {
    int8_t is_live;
    char *bucket_name;
    char *ts_name_prefix;
    char *m3u8_name;
    int32_t video_frame_rate;
    int32_t audio_sample_rate;
    int32_t hls_time;
    int32_t hls_list_size;
} oss_media_hls_stream_options_t;

/**
 * this struct describes the properties of hls stream
 */
typedef struct oss_media_hls_stream_s {
    const oss_media_hls_stream_options_t *options;
    oss_media_hls_file_t *ts_file;
    oss_media_hls_file_t *m3u8_file;
    oss_media_hls_frame_t *video_frame;
    oss_media_hls_frame_t *audio_frame;
    oss_media_hls_m3u8_info_t *m3u8_infos;
    int32_t ts_file_index;
    int64_t current_file_begin_pts;
    int32_t has_aud;
    aos_pool_t *pool;
} oss_media_hls_stream_t;

/**
 *  @brief  open oss media hls stream, this function opens the oss hls stream.
 *  @param[in]  auth_func the func to set access_key_id/access_key_secret
 *  @param[in]  options the option of describes oss hls stream
 *  @return:
 *      On success, a pointer to the oss_media_hls_stream_t
 *      otherwise, a null pointer is returned
 */
oss_media_hls_stream_t* oss_media_hls_stream_open(auth_fn_t auth_func,
        const oss_media_hls_stream_options_t *options);

/**
 *  @brief  write h.264 and aac data
 *  @param[in]  video_buf the data of h.264 video
 *  @param[in]  video_len the dta lenght of h.264 video
 *  @param[in]  audio_buf the data of aac video
 *  @param[in]  audio_len the dta lenght of aac video
 *  @param[in]  stream    the hls stream for store h.264 and aac data
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error. 
 */
int oss_media_hls_stream_write(uint8_t *video_buf,
                              uint64_t video_len,
                              uint8_t *audio_buf,
                              uint64_t audio_len,
                              oss_media_hls_stream_t *stream);

/**
 *  @brief  close oss media hls stream
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error. 
 */
int oss_media_hls_stream_close(oss_media_hls_stream_t *stream);

#endif
