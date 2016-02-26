#include <stdlib.h>
#include <stdio.h>
#include "src/oss_media_ts.h"
#include "src/oss_media_client.h"
#include "config.h"
#include "sample.h"

static void auth_func(oss_media_file_t *file) {
    file->endpoint = SAMPLE_OSS_ENDPOINT;
    file->is_cname = 0;
    file->access_key_id = SAMPLE_ACCESS_KEY_ID;
    file->access_key_secret = SAMPLE_ACCESS_KEY_SECRET;
    file->token = NULL; //SAMPLE_STS_TOKEN; // if use sts token

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

static void do_write(oss_media_ts_file_t *file) {
    oss_media_ts_frame_t frame;
    FILE    *file_h264;
    uint8_t *buf_h264;
    int     len_h264, i;
    int     cur_pos = -1;
    int     last_pos = -1;
    int     video_frame_rate = 30;
    int     max_size = 10 * 1024 * 1024;
    char    *h264_file_name = SAMPLE_DIR"/data/example.h264";

    buf_h264 = calloc(max_size, 1);
    file_h264 = fopen(h264_file_name, "r");
    len_h264 = fread(buf_h264, 1, max_size, file_h264);

    frame.stream_type = st_h264;
    frame.pts = 0;
    frame.continuity_counter = 1;
    frame.key = 1;

    for (i = 0; i < len_h264; i++) {
        if ((buf_h264[i] & 0x0F)==0x00 && buf_h264[i+1]==0x00 
            && buf_h264[i+2]==0x00 && buf_h264[i+3]==0x01) 
        {
            cur_pos = i;
        }

        if (last_pos != -1 && cur_pos > last_pos) {
            frame.pts += 90000 / video_frame_rate;
            frame.dts = frame.pts;
            
            frame.pos = buf_h264 + last_pos;
            frame.end = buf_h264 + cur_pos;

            oss_media_ts_write_frame(&frame, file);
        }

        last_pos = cur_pos;
    }

    fclose(file_h264);
    free(buf_h264);
}

static void write_ts() {
    int ret;
    oss_media_ts_file_t *file;
    
    ret = oss_media_ts_open(SAMPLE_BUCKET_NAME, "oss_media_ts.ts", 
                            auth_func, &file);
    if (ret != 0) {
        printf("open ts file failed.");
        return;
    }

    do_write(file);

    printf("convert H.264 to TS and write to oss file[%s] succeeded\n", 
           file->file->object_key);

    oss_media_ts_close(file);
}

static void write_m3u8() {
    oss_media_ts_file_t *file;
    int ret;

    ret = oss_media_ts_open(SAMPLE_BUCKET_NAME, "oss_media_ts.m3u8", 
                            auth_func, &file);
    if (ret != 0) {
        printf("open m3u8 file failed.");
        return;
    }

    oss_media_ts_m3u8_info_t m3u8[3];
    m3u8[0].duration = 9;
    memcpy(m3u8[0].url, "video-0.ts", strlen("video-0.ts"));
    m3u8[1].duration = 10;
    memcpy(m3u8[1].url, "video-1.ts", strlen("video-1.ts"));
    
    oss_media_ts_begin_m3u8(10, 0, file);
    oss_media_ts_write_m3u8(2, m3u8, file);
    oss_media_ts_end_m3u8(file);

    oss_media_ts_close(file);

    printf("write m3u8 to oss file succeeded\n");
}

static int usage() {
    printf("Usage: oss_media_ts_example type\n"
           "type:\n"
           "     ts\n"
           "     m3u8\n");
    return -1;
}

int main(int argc, char *argv[]) {
    oss_media_init(AOS_LOG_INFO);

    if (argc < 2) {
        return usage();
    }

    if (strcmp("ts", argv[1]) == 0) {
        write_ts();
    } else if (strcmp("m3u8", argv[1]) == 0) {
        write_m3u8();
    } else {
        printf("Unsupport operation:%s\n", argv[1]);
        usage();
    }

    oss_media_destroy();
}
