#include <stdlib.h>
#include <stdio.h>
#include "src/oss_media_ts_stream.h"
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

static void write_only_video_vod() {
    int ret;
    int max_size = 10 * 1024 * 1024;
    FILE *h264_file;
    uint8_t *h264_buf;
    int h264_len;
    
    oss_media_ts_stream_option_t option;
    oss_media_ts_stream_t *stream = NULL;

    option.is_live = 0;
    option.bucket_name = SAMPLE_BUCKET_NAME;
    option.ts_name_prefix = "vod/video/test";
    option.m3u8_name = "vod/video/vod.m3u8";
    option.video_frame_rate = 30;
    option.hls_time = 5;
    
    stream = oss_media_ts_stream_open(auth_func, &option);
    if (stream == NULL) {
        printf("open ts stream failed.\n");
        return;
    }

    h264_buf = malloc(max_size);

    {
        h264_file = fopen(SAMPLE_DIR"/data/slice/video/1.h264", "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, NULL, 0, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }
    }

    {
        h264_file = fopen(SAMPLE_DIR"/data/slice/video/2.h264", "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, NULL, 0, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }
    }
    
    {
        h264_file = fopen(SAMPLE_DIR"/data/slice/video/3.h264", "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, NULL, 0, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }        
    }

    ret = oss_media_ts_stream_close(stream);
    if (ret != 0) {
        printf("close vod stream failed.\n");
        return;
    }
    
    free(h264_buf);
    printf("convert H.264 to TS vod succeeded\n");
}

static void write_only_audio_vod() {
    int ret;
    int max_size = 10 * 1024 * 1024;
    FILE *aac_file;
    uint8_t *aac_buf;
    int aac_len;
    
    oss_media_ts_stream_option_t option;
    oss_media_ts_stream_t *stream = NULL;

    option.is_live = 0;
    option.bucket_name = SAMPLE_BUCKET_NAME;
    option.ts_name_prefix = "vod/audio/test";
    option.m3u8_name = "vod/audio/vod.m3u8";
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    stream = oss_media_ts_stream_open(auth_func, &option);
    if (stream == NULL) {
        printf("open ts stream failed.\n");
        return;
    }

    aac_buf = malloc(max_size);

    {
        aac_file = fopen(SAMPLE_DIR"/data/slice/audio/1.aac", "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);
        
        ret = oss_media_ts_stream_write(NULL, 0, aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }
    }

    {
        aac_file = fopen(SAMPLE_DIR"/data/slice/audio/2.aac", "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);

        ret = oss_media_ts_stream_write(NULL, 0, aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }
    }
    
    {
        aac_file = fopen(SAMPLE_DIR"/data/slice/audio/3.aac", "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);

        ret = oss_media_ts_stream_write(NULL, 0, aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }        
    }

    ret = oss_media_ts_stream_close(stream);
    if (ret != 0) {
        printf("close vod stream failed.\n");
        return;
    }

    free(aac_buf);
    printf("convert aac to TS vod succeeded\n");
}

static void write_video_audio_vod() {
    int ret;
    int max_size = 10 * 1024 * 1024;
    FILE *h264_file, *aac_file;
    uint8_t *h264_buf, *aac_buf;
    int h264_len, aac_len;
    
    oss_media_ts_stream_option_t option;
    oss_media_ts_stream_t *stream = NULL;

    option.is_live = 0;
    option.bucket_name = SAMPLE_BUCKET_NAME;
    option.ts_name_prefix = "vod/video_audio/test";
    option.m3u8_name = "vod/video_audio/vod.m3u8";
    option.video_frame_rate = 30;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    stream = oss_media_ts_stream_open(auth_func, &option);
    if (stream == NULL) {
        printf("open ts stream failed.\n");
        return;
    }

    h264_buf = malloc(max_size);
    aac_buf = malloc(max_size);
    {
        h264_file = fopen(SAMPLE_DIR"/data/slice/video/1.h264", "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        aac_file = fopen(SAMPLE_DIR"/data/slice/audio/1.aac", "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, 
                aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }
    }

    {
        h264_file = fopen(SAMPLE_DIR"/data/slice/video/2.h264", "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        aac_file = fopen(SAMPLE_DIR"/data/slice/audio/1.aac", "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, 
                aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }
    }
    
    {
        h264_file = fopen(SAMPLE_DIR"/data/slice/video/3.h264", "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        aac_file = fopen(SAMPLE_DIR"/data/slice/audio/3.aac", "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, 
                aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write vod stream failed.\n");
            return;
        }        
    }

    ret = oss_media_ts_stream_close(stream);
    if (ret != 0) {
        printf("close vod stream failed.\n");
        return;
    }

    free(h264_buf);
    free(aac_buf);
    printf("convert H.264 and aac to TS vod succeeded\n");
}

static void write_video_audio_live() {
    int ret;
    int max_size = 10 * 1024 * 1024;
    FILE *h264_file, *aac_file;
    uint8_t *h264_buf, *aac_buf;
    int h264_len, aac_len;
    
    oss_media_ts_stream_option_t option;
    oss_media_ts_stream_t *stream = NULL;

    option.is_live = 1;
    option.bucket_name = SAMPLE_BUCKET_NAME;
    option.ts_name_prefix = "live/video_audio/test";
    option.m3u8_name = "live/video_audio/live.m3u8";
    option.video_frame_rate = 30;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    option.hls_list_size = 5;
    
    stream = oss_media_ts_stream_open(auth_func, &option);
    if (stream == NULL) {
        printf("open ts stream failed.\n");
        return;
    }

    int video_index = 1;
    int audio_index = 1;
    h264_buf = malloc(max_size);
    aac_buf = malloc(max_size);
    
    do {
        h264_len = 0;
        aac_len = 0;

        char *file_path = (char*)malloc(128);

        sprintf(file_path, SAMPLE_DIR"/data/live/video/%d.h264",
                video_index);
        h264_file = fopen(file_path, "r");
        h264_len = fread(h264_buf, 1, max_size, h264_file);
        fclose(h264_file);

        sprintf(file_path, SAMPLE_DIR"/data/live/audio/%d.aac",
                audio_index);
        aac_file = fopen(file_path, "r");
        aac_len = fread(aac_buf, 1, max_size, aac_file);
        fclose(aac_file);

        ret = oss_media_ts_stream_write(h264_buf, h264_len, 
                aac_buf, aac_len, stream);
        if (ret != 0) {
            printf("write live stream failed.\n");
            return;
        }
        free(file_path);
        sleep(8);
    } while (video_index++ < 9 && audio_index++ < 9);

    ret = oss_media_ts_stream_close(stream);
    if (ret != 0) {
        printf("close live stream failed.\n");
        return;
    }

    free(h264_buf);
    free(aac_buf);
    printf("convert H.264 and aac to TS live succeeded\n");
}

static int usage() {
    printf("Usage: oss_media_ts_stream_example type\n"
           "type:\n"
           "     only_video_vod\n"
           "     only_audio_vod\n"
           "     video_audio_vod\n"
           "     video_audio_live\n");
    return -1;
}

int main(int argc, char *argv[]) {
    oss_media_init(AOS_LOG_INFO);

    if (argc < 2) {
        return usage();
    }

    if (strcmp("only_video_vod", argv[1]) == 0) {
        write_only_video_vod();
    } else if (strcmp("only_audio_vod", argv[1]) == 0) {
        write_only_audio_vod();
    } else if (strcmp("video_audio_vod", argv[1]) == 0) {
        write_video_audio_vod();
    } else if (strcmp("video_audio_live", argv[1]) == 0) {
        write_video_audio_live();
    } else {
        printf("Unsupport operation:%s\n", argv[1]);
        usage();
    }

    oss_media_destroy();

    return 0;
}
