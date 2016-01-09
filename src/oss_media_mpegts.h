#ifndef OSS_MEDIA_MPEGTS_H
#define OSS_MEDIA_MPEGTS_H

#include <stdint.h>

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
#define OSS_MEDIA_MPEGTS_PACKET_SIZE 188
/* encrypt key size: 32 bytes */
#define OSS_MEDIA_MPEGTS_ENCRYPT_KEY_SIZE 32
/* encrypt packet length */
#define OSS_MEDIA_MPEGTS_ENCRYPT_PACKET_LENGTH 16

/**
 *  this struct describes the memory buffer.
 */
typedef struct oss_media_mpegts_buf_s {
    uint8_t *buf;
    unsigned int pos;
    unsigned int last;
    unsigned int start;
    unsigned int end;
} oss_media_mpegts_buf_t;

/**
 *  this struct describes the mpegts frame infomation.
 */
typedef struct oss_media_mpegts_frame_s {
    uint32_t pid;
    uint32_t sid;
    uint64_t pts;
    uint64_t dts;
    uint32_t cc;
    uint8_t  key:1;
} oss_media_mpegts_frame_t;

/**
 *  this struct describes the m3u8 infomation.
 */
typedef struct oss_media_mpegts_m3u8_info_s {
    int duration;
    char url[256];
} oss_media_mpegts_m3u8_info_t;

/**
 *  mpegts process context.
 */
typedef struct oss_media_mpegts_context_s {
    void *file;
    oss_media_mpegts_buf_t *buffer;
    int (*handler) (struct oss_media_mpegts_context_s *ctx);

    uint8_t encrypt:1;
    char    key[OSS_MEDIA_MPEGTS_ENCRYPT_KEY_SIZE];
} oss_media_mpegts_context_t;

/**
 *  callback function, used for processing local files.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise non-zero is returned
 */
int oss_media_mpegts_file_handler(oss_media_mpegts_context_t *ctx);

/**
 *  callback function, used for processing oss files.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise non-zero is returned
 */
int oss_media_mpegts_ossfile_handler(oss_media_mpegts_context_t *ctx);

/**
 *  open mpegts context.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_mpegts_open(oss_media_mpegts_context_t *ctx);

/**
 *  write mpegts header.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_mpegts_write_header(oss_media_mpegts_context_t *ctx);

/**
 *  write mpegts frame.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_mpegts_write_frame(oss_media_mpegts_context_t *ctx, oss_media_mpegts_frame_t *frame, oss_media_mpegts_buf_t *buf);

/**
 *  write m3u8 infomation.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_mpegts_write_m3u8(oss_media_mpegts_context_t *ctx, oss_media_mpegts_m3u8_info_t m3u8[], int nsize);

/**
 *  close mpegts context.
 *
 *  return values:
 *      upon successful completion 0 is returned
 *      otherwise -1 is returned
 */
int oss_media_mpegts_close(oss_media_mpegts_context_t *ctx);

#endif
