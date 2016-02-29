#ifndef OSS_MEDIA_TS_H
#define OSS_MEDIA_TS_H

#include <stdint.h>
#include "oss_media_client.h"

/* packet size: 188 bytes */
#define OSS_MEDIA_TS_PACKET_SIZE 188
/* encrypt key size: 32 bytes */
#define OSS_MEDIA_TS_ENCRYPT_KEY_SIZE 32
/* encrypt packet length */
#define OSS_MEDIA_TS_ENCRYPT_PACKET_LENGTH 16

#define OSS_MEDIA_M3U8_URL_LENGTH 256

/**
 *  this enum describes the stream type.
 */
typedef enum {
    st_h264,
    st_aac,
    st_mp3
} stream_type_t;

/**
 *  this enum describes the frame type.
 */
typedef enum {
    ft_unspecified = 0,
    ft_non_idr = 1,
    ft_idr = 5,
    ft_sei = 6,
    ft_sps = 7,
    ft_pps = 8,
    ft_aud = 9,
} frame_type_t;

/**
 *  this struct describes the ts frame infomation.
 */
typedef struct oss_media_ts_frame_s {
    stream_type_t stream_type;
    frame_type_t frame_type;
    uint64_t pts;
    uint64_t dts;
    uint32_t continuity_counter;
    uint8_t  key:1;
    uint8_t *pos;
    uint8_t *end;
} oss_media_ts_frame_t;

/**
 *  this typedef define the file_handler_fn_t.
 */
struct oss_media_ts_file_s;
typedef int (*file_handler_fn_t) (struct oss_media_ts_file_s *file);

/**
 *  this struct describes the m3u8 infomation.
 */
typedef struct oss_media_ts_m3u8_info_s {
    float duration;
    char url[OSS_MEDIA_M3U8_URL_LENGTH];
} oss_media_ts_m3u8_info_t;

/**
 *  this struct describes the ts options.
 */
typedef struct oss_media_ts_option_s {
    uint16_t video_pid;
    uint16_t audio_pid;
    uint32_t hls_delay_ms;
    uint8_t encrypt:1;
    char    key[OSS_MEDIA_TS_ENCRYPT_KEY_SIZE];    
    file_handler_fn_t handler_func;
    uint16_t pat_interval_frame_count;
} oss_media_ts_option_t;

/**
 *  this struct describes the memory buffer.
 */
typedef struct oss_media_mpegts_buf_s {
    uint8_t *buf;
    unsigned int pos;
    unsigned int start;
    unsigned int end;
} oss_media_ts_buf_t;

/**
 *  this struct describes the ts file.
 */
typedef struct oss_media_ts_file_s {
    oss_media_file_t *file;
    oss_media_ts_buf_t *buffer;
    oss_media_ts_option_t options;
    uint64_t frame_count;
} oss_media_ts_file_t;

/**
 *  open ts file.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_open(char *bucket_name,
                      char *object_key,
                      auth_fn_t auth_func,
                      oss_media_ts_file_t **file);

/**
 *  write ts frame.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_write_frame(oss_media_ts_frame_t *frame,
                             oss_media_ts_file_t *file);

/**
 *  write m3u8 head infomation.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
void oss_media_ts_begin_m3u8(int32_t max_duration, 
                             int32_t sequence,
                             oss_media_ts_file_t *file);

/**
 *  write m3u8 infomation.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_write_m3u8(int size,
                            oss_media_ts_m3u8_info_t m3u8[],
                            oss_media_ts_file_t *file);

/**
 *  write m3u8 end infomation for vod
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
void oss_media_ts_end_m3u8(oss_media_ts_file_t *file);

/**
 *  flush ts data to oss.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_flush(oss_media_ts_file_t *file);

/**
 *  close ts file.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_close(oss_media_ts_file_t *file);

#endif
