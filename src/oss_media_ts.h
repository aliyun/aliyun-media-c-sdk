#ifndef OSS_MEDIA_TS_H
#define OSS_MEDIA_TS_H

#include <stdint.h>
#include "oss_media_client.h"

/*
 * ================================= TS Protocol =======================================================
 *
 * * TS
 *
 * |<-----------  188 bytes  -------------->|<-----------  188 bytes  -------------->| (Bytes)
 * +--------+-------------------------------+--------+-------------------------------+
 * | Header |         Payload               | Header |           Payload             |
 * +--------+-------------------------------+--------+-------------------------------+
 *
 * * TS Header
 *
 * |<------------  8  ------------->| 1  | 1  | 1  |<-----------  13  ------------>|<-  2 ->|  1 | 1  |<----  4   ---->| (Bits)
 * +--------------------------------+----+----+----+--------------+----------------+--------+----+----+----------------+
 * |         sync byte              |TEI |PUSI| TP |             PID               |   TSC  | AF |  P |       CC       |
 * +--------------------------------+----+----+----+--------------+----------------+--------+----+----+----------------+
 *
 * |<-  8 ->|<---   1   --->|<--    1    -->|<--   1   -->|  1   |   1  |    1   |    1    |     1      | 48  |  48  |<--  8  -->| variable| variable |  (Bits)
 * +--------+---------------+---------------+-------------+------+------+--------+---------+------------+-----+------+-----------+---...---+----------+
 * | Field  | Discontinuity | Random Access | ES Priority | PCR  | OPCR | Splice | Private | Adaptation | PCR | OPCR | Splice    | Private | Stuffing |
 * | Length | Indicator     | Indicator     | Indicator   | Flag | Flag | Flag   | Flag    | Ext Flag   |     |      | Countdown | Data    | Bytes    |
 * +--------+---------------+---------------+-------------+------+------+--------+---------+------------+-----+------+-----------+---...---+----------+
 *
 * * PES
 * +--------+-----------------+--------+-----------------+
 * | Header |       ES        | Header |       ES        |
 * +--------+-----------------+--------+-----------------+
 *
 * * PES Header
 *
 * |<---  3   --->|<-  1 ->|<- 2  ->|  variable  | variable | (Bytes)
 * +--------------+--------+--------+------------+----------+
 * | Packet Start | Stream | Packet | Optional   | Stuffing |
 * | Code  Prefix | ID     | Length | PES Header | Bytes    |
 * +--------------+--------+--------+------------+----------+
 *
 * * Optional PES Header
 *
 * |<- 2  ->|<--  2   -->|<-- 1  -->|<--   1  -->|<--  1  -->|<-- 1  -->|<--  2  -->|  1   |    1    |   4   |<--  8   -->| variable | variable | (Bits)
 * +--------+------------+----------+------------+-----------+----------+-----------+------+---------+--...--+------------+----------+----------+
 * | Marker | Scrambling | Priority | Data Align | Copyright | Original | PTS / DTS | ESCR | ES Rate | Other | PES Header | Optional | Stuffing |
 * | Bits   | Control    |          | Indicator  |           | or Copy  | Indicator | Flag | Flag    | Flags | Length     | Fields   | Bytes    |
 * +--------+------------+----------+------------+-----------+----------+-----------+------+---------+--...--+------------+----------+----------+
 *
 */

/* packet size: 188 bytes */
#define OSS_MEDIA_TS_PACKET_SIZE 188
/* encrypt key size: 32 bytes */
#define OSS_MEDIA_TS_ENCRYPT_KEY_SIZE 32
/* encrypt packet length */
#define OSS_MEDIA_TS_ENCRYPT_PACKET_LENGTH 16

/**
 *  this struct describes the memory buffer.
 */

typedef enum {
    st_h264,
    st_aac
} stream_type_t;

/**
 *  this struct describes the ts frame infomation.
 */
typedef struct oss_media_ts_frame_s {
    stream_type_t stream_type;
    uint64_t pts;
    uint64_t dts;
    uint32_t continuity_counter;
    uint8_t  key:1;
    uint8_t *pos;
    uint8_t *end;
} oss_media_ts_frame_t;

/**
 *  this struct describes the m3u8 infomation.
 */

struct oss_media_ts_file_s;
typedef int (*file_handler_fn_t) (struct oss_media_ts_file_s *file);

/**
 *  ts process context.
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

typedef struct oss_media_mpegts_buf_s {
    uint8_t *buf;
    unsigned int pos;
    unsigned int start;
    unsigned int end;
} oss_media_ts_buf_t;

typedef struct oss_media_ts_file_s {
    oss_media_file_t *file;
    oss_media_ts_buf_t *buffer;
    oss_media_ts_option_t options;
    uint64_t frame_count;
} oss_media_ts_file_t;

/**
 *  open ts context.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_open(auth_fn_t auth_funca,
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
 *  write m3u8 infomation.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */

int oss_media_ts_write_m3u8(int duration,
                            const char *url,
                            oss_media_ts_file_t *file);

/**
 *  close ts context.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_ts_close(oss_media_ts_file_t *file);


#endif
