#ifndef OSS_MEDIA_TS_STREAM_H
#define OSS_MEDIA_TS_STREAM_H

#include "oss_media_ts.h"

/**
 * this struct describes the properties of ts stream options
 */
typedef struct oss_media_ts_stream_option_s {
    int8_t is_live;
    char *bucket_name;
    char *ts_name_prefix;
    char *m3u8_name;
    int32_t video_frame_rate;
    int32_t audio_sample_rate;
    int32_t hls_time;
    int32_t hls_list_size;
} oss_media_ts_stream_option_t;

/**
 * this struct describes the properties of ts stream
 */
typedef struct oss_media_ts_stream_s {
    const oss_media_ts_stream_option_t *option;
    oss_media_ts_file_t *ts_file;
    oss_media_ts_file_t *m3u8_file;
    oss_media_ts_frame_t *video_frame;
    oss_media_ts_frame_t *audio_frame;
    oss_media_ts_m3u8_info_t *m3u8_infos;
    int32_t ts_file_index;
    int64_t current_file_begin_pts;
    aos_pool_t *pool;
} oss_media_ts_stream_t;

/**
 *  @brief  open oss media ts stream, this function opens the oss ts stream.
 *  @param[in]  auth_func the func to set access_key_id/access_key_secret
 *  @param[in]  options the option of describes oss ts stream
 *  @param[out] stream the ts stream
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error. 
 */
int oss_media_ts_stream_open(auth_fn_t auth_func,
                             const oss_media_ts_stream_option_t *options,
                             oss_media_ts_stream_t **stream);

/**
 *  @brief  write h.264 and aac data
 *  @param[in]  video_buf the data of h.264 video
 *  @param[in]  video_len the dta lenght of h.264 video
 *  @param[in]  audio_buf the data of aac video
 *  @param[in]  audio_len the dta lenght of aac video
 *  @param[in]  stream    the ts stream for store h.264 and aac data
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error. 
 */
int oss_media_ts_stream_write(uint8_t *video_buf,
                              uint64_t video_len,
                              uint8_t *audio_buf,
                              uint64_t audio_len,
                              oss_media_ts_stream_t *stream);

/**
 *  @brief  close oss media ts stream
 *  @return:
 *      upon successful completion 0 is returned.
 *      otherwise, -1 is returned and code/messaage in struct of file is set to indicate the error. 
 */
int oss_media_ts_stream_close(oss_media_ts_stream_t *stream);

#endif
