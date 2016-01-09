#include <string.h>
#include <stdio.h>
#include <wolfssl/wolfcrypt/aes.h>
#include "oss_media_mpegts.h"
#include "oss_media_client.h"

/* delay: 700ms */
#define OSS_MEDIA_MPEGTS_HLS_DELAY (700 * 90)

static uint8_t oss_media_mpegts_header[] = {
    /* TS */
    0x47, 0x40, 0x00, 0x10, 0x00,
    /* PSI */
    0x00, 0xb0, 0x0d, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PAT */
    0x00, 0x01, 0xf0, 0x01,
    /* CRC */
    0x2e, 0x70, 0x19, 0x05,
    /* stuffing 167 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    /* TS */
    0x47, 0x50, 0x01, 0x10, 0x00,
    /* PSI */
    0x02, 0xb0, 0x17, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PMT */
    0xe1, 0x00,
    0xf0, 0x00,
    0x1b, 0xe1, 0x00, 0xf0, 0x00,   /* h264 */
    0x0f, 0xe1, 0x01, 0xf0, 0x00,   /* aac */
    /* CRC */
    0x2f, 0x44, 0xb9, 0x9b,         /* crc for h264 & aac */
    /* stuffing 157 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static uint8_t *oss_media_mpegts_write_pcr(uint8_t *p, uint64_t pcr) {
    *p++ = pcr >> 25;
    *p++ = pcr >> 17;
    *p++ = pcr >> 9;
    *p++ = pcr >> 1;
    *p++ = pcr << 7 | 0x7E;
    *p++ = 0;

    return p;
}

static uint8_t *oss_media_mpegts_write_pts(uint8_t *p, uint32_t fb, uint64_t pts) {
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

static void oss_media_mpegts_encrypt_packet(oss_media_mpegts_context_t *ctx, uint8_t *packet) {
    Aes aes;
    wc_AesSetKey(&aes, ctx->key, OSS_MEDIA_MPEGTS_ENCRYPT_KEY_SIZE, NULL, AES_ENCRYPTION);
    wc_AesCbcEncrypt(&aes, packet, packet, OSS_MEDIA_MPEGTS_ENCRYPT_PACKET_LENGTH);
}

static void _oss_media_mpegts_decrypt_packet_debug(oss_media_mpegts_context_t *ctx, uint8_t *packet) {
    int i;
    Aes aes;
    unsigned char out[OSS_MEDIA_MPEGTS_ENCRYPT_PACKET_LENGTH];

    wc_AesSetKey(&aes, ctx->key, OSS_MEDIA_MPEGTS_ENCRYPT_KEY_SIZE, NULL, AES_DECRYPTION);
    wc_AesCbcDecrypt(&aes, out, packet, OSS_MEDIA_MPEGTS_ENCRYPT_PACKET_LENGTH);
    for (i=0; i<4; i++)
        printf("%.2X ", out[i]);
    printf("\n");
}

int oss_media_mpegts_file_handler(oss_media_mpegts_context_t *ctx) {
    if (ctx->buffer->last - ctx->buffer->pos <= 0)
        return 0;
    if (fwrite(&ctx->buffer->buf[ctx->buffer->pos], 1, ctx->buffer->last - ctx->buffer->pos, ctx->file) <= 0)
        return -1;
    return 0;
}

int oss_media_mpegts_ossfile_handler(oss_media_mpegts_context_t *ctx) {
    if (ctx->buffer->last - ctx->buffer->pos <= 0)
        return 0;
    if (oss_media_file_write(ctx->file, &ctx->buffer->buf[ctx->buffer->pos], ctx->buffer->last - ctx->buffer->pos) == -1)
        return -1;
    return 0;
}

int oss_media_mpegts_open(oss_media_mpegts_context_t *ctx) {
    ctx->buffer->pos = ctx->buffer->start;
    ctx->buffer->last = ctx->buffer->start;
    return 0;
}

int oss_media_mpegts_write_header(oss_media_mpegts_context_t *ctx) {
    // failed if buffer is too small.
    if (ctx->buffer->end - ctx->buffer->start < sizeof(oss_media_mpegts_header))
        return -1;

    memcpy(&ctx->buffer->buf[ctx->buffer->last], oss_media_mpegts_header, sizeof(oss_media_mpegts_header));
    ctx->buffer->last += sizeof(oss_media_mpegts_header);

    if (ctx->encrypt) {
        uint8_t *pos;
        pos = &ctx->buffer->buf[ctx->buffer->last];
        pos -= OSS_MEDIA_MPEGTS_PACKET_SIZE;
        oss_media_mpegts_encrypt_packet(ctx, pos);
        // _oss_media_mpegts_decrypt_packet_debug(ctx, pos);
        pos -= OSS_MEDIA_MPEGTS_PACKET_SIZE;
        oss_media_mpegts_encrypt_packet(ctx, pos);
        // _oss_media_mpegts_decrypt_packet_debug(ctx, pos);
    }
    return 0;
}

int oss_media_mpegts_write_frame(oss_media_mpegts_context_t *ctx, oss_media_mpegts_frame_t *frame, oss_media_mpegts_buf_t *buf) {
    uint32_t pes_size, header_size, body_size, in_size, stuff_size, flags;
    uint8_t  packet[OSS_MEDIA_MPEGTS_PACKET_SIZE], *p, *base;
    uint32_t first;

    first = 1;
    memset(packet, 0x00, OSS_MEDIA_MPEGTS_PACKET_SIZE);
    while (buf->pos < buf->last) {
        // failed if buffer is too small.
        if (ctx->buffer->end - ctx->buffer->start < sizeof(packet))
            return -1;

        // flush if buffer is full.
        if (ctx->buffer->end - ctx->buffer->last < sizeof(packet)) {
            if (ctx->handler) {
                if (ctx->handler(ctx) != 0)
                    return -1;

                ctx->buffer->pos = ctx->buffer->start;
                ctx->buffer->last = ctx->buffer->start;
            }
        }
        p = packet;
        frame->cc ++;

        *p++ = 0x47;                        // sync byte
        *p++ = frame->pid >> 8;             // pid
        if (first)
            *(p - 1) |= 0x40;               // first payload
        *p++ = frame->pid;                  // pid
        *p++ = 0x10 | (frame->cc & 0x0F);   // adaptation / payload flag + counter

        if (first) {
            if (frame->key) {
                packet[3] |= 0x20;          // adaptation
                *p++ = 7;                   // size
                *p++ = 0x50;                // random access + pcr

                p = oss_media_mpegts_write_pcr(p, frame->dts - OSS_MEDIA_MPEGTS_HLS_DELAY);
            }

            /* pes header */
            *p ++ = 0x00;                   //-------------------------
            *p ++ = 0x00;                   // packet start code prefix
            *p ++ = 0x01;                   //-------------------------
            *p ++ = frame->sid;             // stream id

            header_size = 5;
            flags = 0x80;                   // pts
            if (frame->dts != frame->pts) {
                header_size += 5;
                flags |= 0x40;              // dts
            }

            pes_size = (buf->last - buf->pos) + header_size + 3;
            if (pes_size > 0xFFFF)
                pes_size = 0;

            *p ++ = pes_size >> 8;      // pes length
            *p ++ = pes_size;           // pes length
            *p ++ = 0x80;               // flags
            *p ++ = flags;              // flags
            *p ++ = header_size;        // pes header size;

            p = oss_media_mpegts_write_pts(p, flags >> 6, frame->pts + OSS_MEDIA_MPEGTS_HLS_DELAY);;

            if (frame->dts != frame->pts) {
                p = oss_media_mpegts_write_pts(p, 1, frame->dts + OSS_MEDIA_MPEGTS_HLS_DELAY);
            }

            first = 0;
        }

        body_size = packet + sizeof(packet) - p;
        in_size = buf->last - buf->pos;

        if (in_size >= body_size) {
            memcpy(p, &buf->buf[buf->pos], body_size);
            buf->pos += body_size;
        } else {
            stuff_size = body_size - in_size;

            if (packet[3] & 0x20) { // has adaptation
                base = &packet[5] + packet[4];
                p = memmove(base + stuff_size, base, p - base);
                memset(base, 0xFF, stuff_size);
                packet[4] += stuff_size;
            } else {                // no  adaptation
                packet[3] |= 0x20;
                p = memmove(&packet[4] + stuff_size, &packet[4], p-&packet[4]);
                packet[4] = stuff_size - 1;
                if (stuff_size >=2) {
                    packet[5] = 0;
                    memset(&packet[6], 0xFF, stuff_size - 2);
                }
            }

            memcpy(&packet[OSS_MEDIA_MPEGTS_PACKET_SIZE]-in_size, &buf->buf[buf->pos], in_size);
            buf->pos = buf->last;
        }

        if (ctx->encrypt) {
            oss_media_mpegts_encrypt_packet(ctx, packet);
            // _oss_media_mpegts_decrypt_packet_debug(ctx, packet);
        }

        memcpy(&ctx->buffer->buf[ctx->buffer->last], packet, sizeof(packet));
        ctx->buffer->last += sizeof(packet);
    }

    return 0;
}

int oss_media_mpegts_write_m3u8(oss_media_mpegts_context_t *ctx, oss_media_mpegts_m3u8_info_t m3u8[], int nsize) {
    char *header = "#EXTM3U\n\n";
    char info[256 + 32];
    int i;
    // failed if buffer is too small.
    if (ctx->buffer->end - ctx->buffer->start < 512)
        return -1;

    memcpy(&ctx->buffer->buf[ctx->buffer->last], header, strlen(header));
    ctx->buffer->last += strlen(header);
    for (i=0; i<nsize; i++) {
        sprintf(info, "#EXTINF:%d\n%s\n\n", m3u8[i].duration, m3u8[i].url);
        // flush if full.
        if (ctx->buffer->end - ctx->buffer->last < strlen(info)) {
            if (ctx->handler) {
                if (ctx->handler(ctx) != 0)
                    return -1;

                ctx->buffer->pos = ctx->buffer->start;
                ctx->buffer->last = ctx->buffer->start;
            }
        }

        memcpy(&ctx->buffer->buf[ctx->buffer->last], info, strlen(info));
        ctx->buffer->last += strlen(info);
    }
    return 0;
}

int oss_media_mpegts_close(oss_media_mpegts_context_t *ctx) {
    if (ctx->handler) {
        if (ctx->handler(ctx) != 0)
            return -1;

        ctx->buffer->pos = ctx->buffer->start;
        ctx->buffer->last = ctx->buffer->start;
    }

    return 0;
}
