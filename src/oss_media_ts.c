#include <string.h>
#include <math.h>
#include <stdio.h>
#include <wolfssl/wolfcrypt/aes.h>
#include "oss_media_ts.h"
#include "oss_media_client.h"

/* delay: 700ms */
#define OSS_MEDIA_TS_HLS_DELAY (700 * 90)

#define OSS_MEDIA_TS_HEADER_LENGTH 5

static uint32_t crc_table[256];

static void make_crc_table(void)
{
    uint32_t i;
    for(i = 0; i < 256; i++ ) {
        uint32_t crc = 0;
        uint32_t j;
        for(j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1 ) {
            crc = (crc << 1) ^ (((crc ^ j) & 0x80000000) ? 0x04c11db7 : 0);
        }

        crc_table[i] = crc;
    }    
}

static uint32_t calculate_crc32(uint8_t *data, int32_t length) {
    static int table_made = 0;
    if (!table_made) make_crc_table();  

    uint32_t i;
    uint32_t crc32 = 0xFFFFFFFF;
    for (i = 0; i < length; i++) {
        crc32 = (crc32 << 8 ) ^ crc_table[((crc32 >> 24) ^ *data++) & 0xFF];
    }
    return crc32;
}

static uint8_t *oss_media_ts_write_pcr(uint8_t *p, uint64_t pcr) {
    *p++ = pcr >> 25;
    *p++ = pcr >> 17;
    *p++ = pcr >> 9;
    *p++ = pcr >> 1;
    *p++ = pcr << 7 | 0x7E;
    *p++ = 0;

    return p;
}

static uint8_t *oss_media_ts_write_pts(uint8_t *p, uint32_t fb, uint64_t pts) {
    uint32_t val;
    
    val = fb << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *p++ = val;

    val = (((pts >> 15) & 0x7FFF) << 1) | 1;
    *p++ = (val >> 8);
    *p++ = val;

    val = (((pts) & 0x7FFF) << 1) | 1;
    *p++ = (val >> 8);
    *p++ = val;

    return p;
}

static void oss_media_ts_encrypt_packet(oss_media_ts_file_t *file, uint8_t *packet) {
    Aes aes;
    wc_AesSetKey(&aes, file->options.key, OSS_MEDIA_TS_ENCRYPT_KEY_SIZE, 
                 NULL, AES_ENCRYPTION);
    wc_AesCbcEncrypt(&aes, packet, packet, OSS_MEDIA_TS_ENCRYPT_PACKET_LENGTH);
}

static int oss_media_ts_write_pat(oss_media_ts_file_t *file) {
    if (file->buffer->end - file->buffer->pos < OSS_MEDIA_TS_PACKET_SIZE) {
        if (file->options.handler_func(file) != 0) {
            return -1;
        }
    }

    uint8_t *start = file->buffer->buf + file->buffer->pos;
    uint8_t *p = start;

    // HLS header
    uint8_t sync_byte = 0x47;
    uint8_t transport_error_indicator = 0;
    uint8_t payload_unit_start_indicator = 1;
    uint8_t transport_priority = 0;
    uint16_t pat_pid = 0;
    uint8_t transport_scrambling_control = 0;
    uint8_t adaptation_field_control = 1;
    uint8_t continuity_counter = 0;

    *p++ = sync_byte;
    *p++ = (transport_error_indicator << 7) | (payload_unit_start_indicator << 6) 
           | (transport_priority << 5) | (pat_pid >> 8) & 0x1F;
    *p++ = pat_pid & 0xff;
    *p++ = (transport_scrambling_control << 6) | (adaptation_field_control << 4)
           | (continuity_counter & 0x0F);
    *p++ = 0x00;

    // PAT
    uint8_t section_syntax_indicator = 1;
    uint8_t zero = 0;
    uint8_t reserved_1 = 3;
    uint16_t section_length = 13; 
    uint16_t transport_stream_id = 1;
    uint8_t reserved_2 = 3;
    uint8_t version_number = 0;
    uint8_t current_next_indicator = 1;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint16_t program_number = 1;
    uint8_t reserved_3 = 7;
    uint16_t program_id = 4097;

    *p++ = pat_pid & 0XFF;
    *p++ = (section_syntax_indicator << 7) | (zero << 6)
           | (reserved_1 << 4) | ((section_length >> 8) & 0x0F);
    *p++ = section_length & 0xFF;
    *p++ = (transport_stream_id >> 8) & 0xFF;
    *p++ = transport_stream_id & 0xFF;
    *p++ = (reserved_2 << 6) | (version_number << 1) 
           | (current_next_indicator & 0x01);
    *p++ = section_number & 0xFF;
    *p++ = last_section_number & 0xFF;
    *p++ = (program_number >> 8) &0xFF;
    *p++ = program_number &0xFF;
    *p++ = (reserved_3 << 5) | ((program_id >> 8) & 0x1F);
    *p++ = program_id & 0xFF;

    // calculate crc32
    uint32_t crc32 = calculate_crc32(start + OSS_MEDIA_TS_HEADER_LENGTH, 
            p - start - OSS_MEDIA_TS_HEADER_LENGTH);
    *p++ = crc32 >> 24;
    *p++ = (crc32 >> 16) & 0xff;
    *p++ = (crc32 >> 8) & 0xff;
    *p++ = crc32 & 0xff;

    // fill 0xFF
    while (p - start < OSS_MEDIA_TS_PACKET_SIZE) {
        *p++ = 0xff;
    }
    file->buffer->pos += OSS_MEDIA_TS_PACKET_SIZE;
    
    return 0;
}

static int oss_media_ts_write_pmt(oss_media_ts_file_t *file) {
    if (file->buffer->end - file->buffer->pos < OSS_MEDIA_TS_PACKET_SIZE) {
        if (file->options.handler_func(file) != 0) {
            return -1;
        }
    }

    uint8_t *start = file->buffer->buf + file->buffer->pos;
    uint8_t *p = start;

    // HLS header
    uint8_t sync_byte = 0x47;
    uint8_t transport_error_indicator = 0;
    uint8_t payload_unit_start_indicator = 1;
    uint8_t transport_priority = 0;
    uint16_t pmt_pid = 4097;
    uint8_t transport_scrambling_control = 0;
    uint8_t adaptation_field_control = 1;
    uint8_t continuity_counter = 0;

    *p++ = sync_byte;
    *p++ = (transport_error_indicator << 7) | (payload_unit_start_indicator << 6) 
           | (transport_priority << 5) | (pmt_pid >> 8) & 0x1F;
    *p++ = pmt_pid & 0xff;
    *p++ = (transport_scrambling_control << 6) | (adaptation_field_control << 4)
           | (continuity_counter & 0x0F);
    *p++ = 0x00;

    // PMT
    uint8_t table_id = 0x02;
    uint8_t section_syntax_indicator = 1;
    uint8_t zero = 0;
    uint8_t reserved_1 = 3;
    uint16_t section_length = 23;
    uint16_t program_number = 1;
    uint8_t reserved_2 = 3;
    uint8_t version_number = 0;
    uint8_t current_next_indicator = 1;
    uint8_t section_number = 0;
    uint8_t last_section_number = 0;
    uint8_t reserved_3 = 7;
    uint16_t pcr_pid = file->options.video_pid;
    uint8_t reserved_4 = 15;
    uint16_t program_info_length = 0;

    *p++ = table_id;
    *p++ = (section_syntax_indicator << 7) | (zero << 6) | (reserved_1 << 4)
           | ((section_length >> 8) & 0x0F);
    *p++ = section_length & 0xFF;
    *p++ = (program_number >> 8) & 0xFF;
    *p++ = program_number & 0xFF;
    *p++ = (reserved_2 << 6) | (version_number << 1) 
           | (current_next_indicator & 0x01);
    *p++ = section_number;
    *p++ = last_section_number;
    *p++ = (reserved_3 << 5) | ((pcr_pid >> 8) & 0xFF);
    *p++ = pcr_pid & 0xFF;
    *p++ = (reserved_4 << 4) | ((program_info_length >> 8) & 0xFF);
    *p++ = program_info_length & 0xFF;

    {
        uint8_t stream_type = 0x1b;
        uint8_t reserved_5 = 7;
        uint16_t elementary_pid = file->options.video_pid;
        uint8_t reserved_6 = 15;
        uint16_t ES_info_length = 0;

        *p++ = stream_type;
        *p++ = (reserved_5 << 5) | ((elementary_pid >> 8) & 0x1F);
        *p++ = elementary_pid & 0xFF;
        *p++ = (reserved_6 << 4) | ((ES_info_length >> 4) & 0x0F);
        *p++ = ES_info_length & 0xFF;                
    }

    {
        uint8_t stream_type = 0x0f;
        uint8_t reserved_5 = 7;
        uint16_t elementary_pid = file->options.audio_pid;
        uint8_t reserved_6 = 15;
        uint16_t ES_info_length = 0;

        *p++ = stream_type;
        *p++ = (reserved_5 << 5) | ((elementary_pid >> 8) & 0x1F);
        *p++ = elementary_pid & 0xFF;
        *p++ = (reserved_6 << 4) | ((ES_info_length >> 4) & 0x0F);
        *p++ = ES_info_length & 0xFF;                
    }
    
    // calculate CRC32
    uint32_t crc32 = calculate_crc32(start + OSS_MEDIA_TS_HEADER_LENGTH, 
            p - start - OSS_MEDIA_TS_HEADER_LENGTH);
    *p++ = crc32 >> 24;
    *p++ = (crc32 >> 16) & 0xFF;
    *p++ = (crc32 >> 8) & 0xFF;
    *p++ = crc32 & 0xFF;

    // fill 0xFF
    while (p - start < OSS_MEDIA_TS_PACKET_SIZE) {
        *p++ = 0xff;
    }
    file->buffer->pos += OSS_MEDIA_TS_PACKET_SIZE;

    return 0;
}

static int oss_media_ts_write_pat_and_pmt(oss_media_ts_file_t *file) {
    if (0 != oss_media_ts_write_pat(file)) {
        return -1;
    }

    if (0 != oss_media_ts_write_pmt(file)) {
        return -1;
    }

    if (file->options.encrypt) {
        uint8_t *pos;
        pos = &file->buffer->buf[file->buffer->pos];
        pos -= OSS_MEDIA_TS_PACKET_SIZE;
        oss_media_ts_encrypt_packet(file, pos);
        pos -= OSS_MEDIA_TS_PACKET_SIZE;
        oss_media_ts_encrypt_packet(file, pos);
    }

    return 0;
}

static int oss_media_ts_ossfile_handler(oss_media_ts_file_t *file) {
    int64_t length;
    int64_t write_size;

    length = file->buffer->pos - file->buffer->start;
    if (length <= 0) {
        return 0;
    }

    write_size = oss_media_file_write(file->file, 
            &file->buffer->buf[file->buffer->start], length);
    if (write_size != length) {
        aos_error_log("write data to oss file[%s] failed.", 
                      file->file->object_key);
        return -1;
    }

    file->buffer->pos = file->buffer->start;
    
    return 0;
}

static int need_write_pat_and_pmt(oss_media_ts_file_t *file, 
                                  oss_media_ts_frame_t *frame) 
{
    return file->frame_count % file->options.pat_interval_frame_count == 0;
}

static int ends_with(const char *str, const char *surfix) {
    return strncmp(str + strlen(str) - strlen(surfix), surfix, strlen(surfix)) == 0;
}

int oss_media_ts_open(char *bucket_name,
                      char *object_key,
                      auth_fn_t auth_func,
                      oss_media_ts_file_t **file)
{
    *file = (oss_media_ts_file_t*)malloc(sizeof(oss_media_ts_file_t));
    
    (*file)->file = oss_media_file_open(bucket_name, object_key, "a", auth_func);

    (*file)->frame_count = 0;
    (*file)->options.video_pid = OSS_MEDIA_DEFAULT_VIDEO_PID;
    (*file)->options.audio_pid = OSS_MEDIA_DEFAULT_AUDIO_PID;
    (*file)->options.hls_delay_ms = OSS_MEDIA_TS_HLS_DELAY;
    (*file)->options.encrypt = 0;
    (*file)->options.handler_func = oss_media_ts_ossfile_handler;
    (*file)->options.pat_interval_frame_count = OSS_MEDIA_PAT_INTERVAL_FRAME_COUNT;
    
    (*file)->buffer = (oss_media_ts_buf_t*)malloc(sizeof(oss_media_ts_buf_t));
    (*file)->buffer->start = 0;
    (*file)->buffer->pos = 0;
    (*file)->buffer->end = OSS_MEDIA_DEFAULT_WRITE_BUFFER; // pos in [start, end)
    
    if (ends_with(object_key, OSS_MEDIA_TS_FILE_SURFIX)) {
        (*file)->buffer->buf = (uint8_t*)malloc(OSS_MEDIA_DEFAULT_WRITE_BUFFER);
    } else if (ends_with(object_key, OSS_MEDIA_M3U8_FILE_SURFIX)) {
        (*file)->buffer->buf = (uint8_t*)malloc(OSS_MEDIA_DEFAULT_WRITE_BUFFER / 64);
    } else {
        return -1;
    }

    return 0;
}

int oss_media_ts_write_frame(oss_media_ts_frame_t *frame,
                             oss_media_ts_file_t *file)
{
    uint32_t pes_size, header_size, body_size, in_size, stuff_size, flags;
    uint8_t  packet[OSS_MEDIA_TS_PACKET_SIZE], *p, *base;
    uint32_t first;
    uint32_t pid;

    // write pat and pmt table
    if (need_write_pat_and_pmt(file, frame)) {
        oss_media_ts_write_pat_and_pmt(file);
    }

    // get pid
    if (frame->stream_type == st_h264) {
        pid = file->options.video_pid;
    } else if (frame->stream_type == st_aac) {
        pid = file->options.audio_pid;
    } else {
        return -1;      
    }

    first = 1;
    memset(packet, 0x00, OSS_MEDIA_TS_PACKET_SIZE);
    while (frame->pos < frame->end) {
        // flush if buffer is full.
        if (file->buffer->end - file->buffer->pos < sizeof(packet)) {
            if (file->options.handler_func(file) != 0) {
                return -1;
            }
        }

        p = packet;

        *p++ = 0x47;                        // sync byte
        *p++ = pid >> 8;                    // pid
        if (first) {
            *(p - 1) |= 0x40;               // first payload
        }
        *p++ = pid;                         // pid
        *p++ = 0x10 | (frame->continuity_counter++ & 0x0F);

        if (first) {
            if (frame->key) {
                packet[3] |= 0x20;          // adaptation
                *p++ = 7;                   // size
                *p++ = 0x50;                // random access + pcr
                
                p = oss_media_ts_write_pcr(p, 
                        frame->dts - file->options.hls_delay_ms);
            }
            
            /* pes header */
            *p++ = 0x00;                   //-------------------------
            *p++ = 0x00;                   // packet start code pref
            *p++ = 0x01;                   //-------------------------
            *p++ = frame->stream_type == st_h264 ? 0xe0 : 0xc0;
            
            header_size = 5;
            flags = 0x80;                   // pts
            if (frame->stream_type == st_h264) {
                header_size += 5;
                flags |= 0x40;              // dts
            }

            pes_size = (frame->end - frame->pos) + header_size + 3;
            if (pes_size > 0xFFFF) {
                pes_size = 0;
            }

            *p++ = pes_size >> 8;      // pes length
            *p++ = pes_size;           // pes length
            *p++ = 0x80;               // flags
            *p++ = flags;              // flags
            *p++ = header_size;        // pes header size;

            p = oss_media_ts_write_pts(p, flags >> 6, 
                    frame->pts + file->options.hls_delay_ms); 

            if (frame->stream_type == st_h264) {
                p = oss_media_ts_write_pts(p, 1, 
                        frame->dts + file->options.hls_delay_ms); 
            }
            
            first = 0;
        }

        body_size = packet + sizeof(packet) - p;
        in_size = frame->end - frame->pos;

        if (in_size >= body_size) {
            memcpy(p, frame->pos, body_size);
            p += body_size;
            frame->pos += body_size;
        } else {
            stuff_size = body_size - in_size;

            if (packet[3] & 0x20) { // has adaptation
                base = &packet[5] + packet[4];
                p = memmove(base + stuff_size, base, p - base);
                memset(base, 0xFF, stuff_size);
                packet[4] += stuff_size;
            } else {                // no  adaptation
                packet[3] |= 0x20;
                p = memmove(&packet[4] + stuff_size, &packet[4], p - &packet[4]);
                packet[4] = stuff_size - 1;
                if (stuff_size >=2) {
                    packet[5] = 0;
                    memset(&packet[6], 0xFF, stuff_size - 2);
                }
            }
            
            memcpy(&packet[OSS_MEDIA_TS_PACKET_SIZE] - in_size, frame->pos, in_size);
            frame->pos = frame->end;
        }
        
        if (file->options.encrypt) {
            oss_media_ts_encrypt_packet(file, packet);
        }

        memcpy(&file->buffer->buf[file->buffer->pos], packet, sizeof(packet));
        file->buffer->pos += sizeof(packet);
    }
    file->frame_count++;

    return 0;
}

void oss_media_ts_begin_m3u8(int32_t max_duration, 
                             int32_t sequence,
                             oss_media_ts_file_t *file) 
{
    static const char *header = "#EXTM3U\n"
                                "#EXT-X-TARGETDURATION:%d\n"
                                "#EXT-X-MEDIA-SEQUENCE:%d\n"
                                "#EXT-X-VERSION:3\n";
        
    char m3u8_header[strlen(header) + 10];
    sprintf(m3u8_header, header, max_duration, sequence);
        
    memcpy(&file->buffer->buf[file->buffer->pos], m3u8_header, 
           strlen(m3u8_header));
    file->buffer->pos += strlen(m3u8_header);
}

void oss_media_ts_end_m3u8(oss_media_ts_file_t *file) {
    static const char *end = "#EXT-X-ENDLIST";
    memcpy(&file->buffer->buf[file->buffer->pos], end, strlen(end));
    file->buffer->pos += strlen(end);
}

int oss_media_ts_write_m3u8(int size,
                            oss_media_ts_m3u8_info_t m3u8[],
                            oss_media_ts_file_t *file)
{
    int i;
    for (i = 0;i < size; i++) {
        char item[256 + 32];
        sprintf(item, "#EXTINF:%.3f,\n%s\n", m3u8[i].duration, m3u8[i].url);
        memcpy(&file->buffer->buf[file->buffer->pos], item, strlen(item));
        file->buffer->pos += strlen(item);
    }
    
    if (file->options.handler_func(file) != 0) {
        return -1;
    }

    return 0;
}

int oss_media_ts_flush(oss_media_ts_file_t *file) {
    return file->options.handler_func(file);
}

int oss_media_ts_close(oss_media_ts_file_t *file) {
    if (oss_media_ts_flush(file) != 0) {
        aos_error_log("flush file[%s] failed.", file->file->object_key);
        return -1;
    }
        
    oss_media_file_close(file->file);

    free(file->buffer->buf);
    free(file->buffer);

    free(file);
    file = NULL;

    return 0;
}
