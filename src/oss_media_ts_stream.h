#ifndef OSS_MEDIA_TS_STREAM_H
#define OSS_MEDIA_TS_STREAM_H

#include "oss_media_ts.h"

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

int oss_media_ts_stream_open(auth_fn_t auth_func,
                             const oss_media_ts_stream_option_t *options,
                             oss_media_ts_stream_t **stream);

int oss_media_ts_stream_write(uint8_t *video_buf,
                              uint64_t video_len,
                              uint8_t *audio_buf,
                              uint64_t audio_len,
                              oss_media_ts_stream_t *stream);

int oss_media_ts_stream_close(oss_media_ts_stream_t *stream);

#endif
