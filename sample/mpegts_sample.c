#include <stdlib.h>
#include <stdio.h>
#include "src/oss_media_mpegts.h"
#include "src/oss_media_client.h"
#include "config.h"
#include "sample.h"

static void auth_func(oss_media_file_t *file) {
    file->host = SAMPLE_OSS_HOST;
    file->is_oss_domain = 1; //oss host type, host is oss domain ? 1 : 0(cname etc)
    file->bucket = SAMPLE_BUCKET_NAME;
    file->filename = "oss_media_mpegts.ts";
    file->access_key_id = SAMPLE_ACCESS_KEY_ID;
    file->access_key_secret = SAMPLE_ACCESS_KEY_SECRET;
    file->token = NULL; //SAMPLE_STS_TOKEN; // if use sts token

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

static void do_write(oss_media_mpegts_context_t *ctx) {
    oss_media_mpegts_frame_t frame;
    oss_media_mpegts_buf_t buf;
    FILE    *file_h264;
    uint8_t *buf_h264;
    int     len_h264, i;
    int     cur_pos, last_pos;
    int     max_size = 10 * 1024 * 1024;   // 10M
    char    *h264_file_name = SAMPLE_DIR"/data/example.h264";

    // encrypt
    ctx->encrypt = 1;
    memcpy(ctx->key, "encrypt", 7);

    oss_media_mpegts_open(ctx);
    oss_media_mpegts_write_header(ctx);

    buf_h264 = calloc(max_size, 1);
    file_h264 = fopen(h264_file_name, "r");
    len_h264 = fread(buf_h264, 1, max_size, file_h264);

    frame.pid = 0x0100;
    frame.sid = 0xE0;
    frame.pts = 1000;
    frame.cc  = 0;
    frame.key = 1;

    buf.buf = buf_h264;
    buf.start = 0;
    buf.end = len_h264;
    for (i = 0; i < len_h264; i++) {
        if ((buf_h264[i] & 0x0F)==0x00 && buf_h264[i+1]==0x00 
            && buf_h264[i+2]==0x00 && buf_h264[i+3]==0x01) 
        {
            cur_pos = i;
        }

        if (last_pos != -1 && cur_pos > last_pos) {
            frame.pts += 33 * 90;
            frame.dts = frame.pts;
            frame.cc ++;

            buf.pos = last_pos;
            buf.last = cur_pos;

            oss_media_mpegts_write_frame(ctx, &frame, &buf);
        }

        last_pos = cur_pos;
    }
    fclose(file_h264);
    free(buf_h264);

    oss_media_mpegts_close(ctx);
}

static void write_local_file() {
    oss_media_mpegts_context_t *ctx;
    FILE *file;
    char *ts_file_name = "test.ts";

    ctx = calloc(sizeof(oss_media_mpegts_context_t), 1);

    // buffer
    ctx->buffer = calloc(sizeof(oss_media_mpegts_buf_t), 1);
    ctx->buffer->pos = 0;
    ctx->buffer->last = 0;
    ctx->buffer->start = 0;
    ctx->buffer->end = 1024 * 1024;
    ctx->buffer->buf = calloc(1024 * 1024, 1);

    // file
    file = fopen(ts_file_name, "w");
    ctx->file = file;
    ctx->handler = oss_media_mpegts_file_handler;

    do_write(ctx);

    fclose(file);
    free(ctx->buffer->buf);
    free(ctx->buffer);
    free(ctx);
    
    printf("convert H.264 to TS and write to local file[%s] succeeded\n", 
           ts_file_name);
}

static void write_oss_file() {
    oss_media_mpegts_context_t *ctx;
    oss_media_file_t *file;

    ctx = calloc(sizeof(oss_media_mpegts_context_t), 1);

    // buffer
    ctx->buffer = calloc(sizeof(oss_media_mpegts_buf_t), 1);
    ctx->buffer->pos = 0;
    ctx->buffer->last = 0;
    ctx->buffer->start = 0;
    ctx->buffer->end = 1024 * 1024;
    ctx->buffer->buf = calloc(1024 * 1024, 1);

    // file
    file = oss_media_file_create();
    file->auth = auth_func;
    oss_media_file_open(file, "a");
    ctx->file = file;
    ctx->handler = oss_media_mpegts_ossfile_handler;

    do_write(ctx);

    oss_media_file_close(file);
    free(ctx->buffer->buf);
    free(ctx->buffer);
    free(ctx);

    printf("convert H.264 to TS and write to oss file[%s] succeeded\n", 
           file->filename);

    oss_media_file_free(file);
}

static void write_m3u8() {
    oss_media_mpegts_context_t *ctx;
    FILE *file;
    char *m3u8_file = "test.m3u8";
    int i;

    ctx = calloc(sizeof(oss_media_mpegts_context_t), 1);

    // buffer
    ctx->buffer = calloc(sizeof(oss_media_mpegts_buf_t), 1);
    ctx->buffer->pos = 0;
    ctx->buffer->last = 0;
    ctx->buffer->start = 0;
    ctx->buffer->end = 512;
    ctx->buffer->buf = calloc(512, 1);

    // file
    file = fopen(m3u8_file, "w");
    ctx->file = file;
    ctx->handler = oss_media_mpegts_file_handler;

    oss_media_mpegts_m3u8_info_t m3u8[16];
    for (i = 0; i < 16; i++) {
        m3u8[i].duration = i;
        sprintf(m3u8[i].url, "http://example.com/video-%d.ts", i);
    }

    oss_media_mpegts_open(ctx);
    oss_media_mpegts_write_m3u8(ctx, m3u8, 16);
    oss_media_mpegts_close(ctx);

    printf("write m3u8 to local file[%s] succeeded\n", m3u8_file);
}

static int usage() {
    printf("Usage: oss_media_mpegts_example type\n"
           "type:\n"
           "     local_file\n"
           "     oss_file\n"
           "     m3u8\n");
    return -1;
}

int main(int argc, char *argv[]) {
    oss_media_init(AOS_LOG_INFO);

    if (argc < 2) {
        return usage();
    }

    if (strcmp("local_file", argv[1]) == 0) {
        write_local_file();
    } else if (strcmp("oss_file", argv[1]) == 0) {
        write_oss_file();
    } else if (strcmp("m3u8", argv[1]) == 0) {
        write_m3u8();
    } else {
        printf("Unsupport operation:%s\n", argv[1]);
        usage();
    }

    oss_media_destroy();
}
