#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_ts_stream.c"
#include <oss_c_sdk/aos_define.h>

extern void delete_file(oss_media_file_t *file);
static void auth_func(oss_media_file_t *file);
static int oss_media_ts_fake_handler(oss_media_ts_file_t *file);

void test_ts_stream_setup(CuTest *tc) {
    aos_pool_t *p;
    aos_pool_create(&p, NULL);
    
    apr_dir_make(TEST_DIR"/data/", APR_OS_DEFAULT, p);

    aos_pool_destroy(p);
}

void test_ts_stream_teardown(CuTest *tc) {
    aos_pool_t *p;
    aos_pool_create(&p, NULL);
    apr_dir_remove(TEST_DIR"/data/", p);
    aos_pool_destroy(p);
}

void test_oss_media_get_digit_num(CuTest *tc) {
    int num;
    
    num = oss_media_get_digit_num(0);
    CuAssertIntEquals(tc, 1, num);

    num = oss_media_get_digit_num(10);
    CuAssertIntEquals(tc, 2, num);
}

void test_oss_media_ts_stream_open_with_vod(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 700;
    
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    CuAssertIntEquals(tc, 1, stream->ts_file_index);
    CuAssertIntEquals(tc, 0, stream->current_file_begin_pts + 1);
    CuAssertTrue(tc, NULL != stream->option);
    CuAssertStrEquals(tc, "test0.ts", stream->ts_file->file->object_key);
    CuAssertStrEquals(tc, "test.m3u8", stream->m3u8_file->file->object_key);
    CuAssertStrEquals(tc, "a", stream->m3u8_file->file->mode);
    CuAssertIntEquals(tc, st_h264, stream->video_frame->stream_type);
    CuAssertIntEquals(tc, 1, stream->video_frame->continuity_counter);
    CuAssertIntEquals(tc, 1, stream->video_frame->key);
    CuAssertIntEquals(tc, 5000, stream->video_frame->pts);
    CuAssertIntEquals(tc, 5000, stream->video_frame->dts);
    CuAssertTrue(tc, NULL == stream->video_frame->pos);
    CuAssertTrue(tc, NULL == stream->video_frame->end);
    CuAssertIntEquals(tc, st_aac, stream->audio_frame->stream_type);
    CuAssertIntEquals(tc, 1, stream->audio_frame->continuity_counter);
    CuAssertIntEquals(tc, 1, stream->audio_frame->key);
    CuAssertIntEquals(tc, 5000, stream->audio_frame->pts);
    CuAssertIntEquals(tc, 5000, stream->audio_frame->dts);
    CuAssertTrue(tc, NULL == stream->audio_frame->pos);
    CuAssertTrue(tc, NULL == stream->audio_frame->end);
    CuAssertTrue(tc, NULL != stream->pool);

    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_open_with_live(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test.m3u8";
    option.is_live = 1;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 700;
    option.hls_list_size = 5;
    
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    CuAssertIntEquals(tc, 1, stream->ts_file_index);
    CuAssertIntEquals(tc, 0, stream->current_file_begin_pts + 1);
    CuAssertTrue(tc, NULL != stream->option);
    CuAssertStrEquals(tc, "test0.ts", stream->ts_file->file->object_key);
    CuAssertStrEquals(tc, "test.m3u8", stream->m3u8_file->file->object_key);
    CuAssertStrEquals(tc, "w", stream->m3u8_file->file->mode);
    CuAssertIntEquals(tc, st_h264, stream->video_frame->stream_type);
    CuAssertIntEquals(tc, 1, stream->video_frame->continuity_counter);
    CuAssertIntEquals(tc, 1, stream->video_frame->key);
    CuAssertIntEquals(tc, 5000, stream->video_frame->pts);
    CuAssertIntEquals(tc, 5000, stream->video_frame->dts);
    CuAssertTrue(tc, NULL == stream->video_frame->pos);
    CuAssertTrue(tc, NULL == stream->video_frame->end);
    CuAssertIntEquals(tc, st_aac, stream->audio_frame->stream_type);
    CuAssertIntEquals(tc, 1, stream->audio_frame->continuity_counter);
    CuAssertIntEquals(tc, 1, stream->audio_frame->key);
    CuAssertIntEquals(tc, 5000, stream->audio_frame->pts);
    CuAssertIntEquals(tc, 5000, stream->audio_frame->dts);
    CuAssertTrue(tc, NULL == stream->audio_frame->pos);
    CuAssertTrue(tc, NULL == stream->audio_frame->end);
    CuAssertTrue(tc, NULL != stream->pool);

    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_open_with_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test";
    option.bucket_name = "not.exist";
    option.m3u8_name = "test.m3u8";
    option.is_live = 1;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 700;
    option.hls_list_size = 5;
    
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream == NULL);
}

void test_oss_media_create_ts_full_url(CuTest *tc) {
    aos_pool_t *pool;
    aos_pool_create(&pool, NULL);

    oss_media_file_t *file;
    file = (oss_media_file_t*)malloc(sizeof(oss_media_file_t));
    file->endpoint = "oss.abc.com";
    file->bucket_name = "bucket-1";
    file->object_key = "key-1";
    file->is_cname = 0;
    file->mode = "w";

    char *url = oss_media_create_ts_full_url(pool, file);
    CuAssertStrEquals(tc, "http://bucket-1.oss.abc.com/key-1", url);

    aos_pool_destroy(pool);
    oss_media_file_close(file);
}

void test_oss_media_create_ts_full_url_with_prefix(CuTest *tc) {
    aos_pool_t *pool;
    aos_pool_create(&pool, NULL);

    oss_media_file_t *file;
    file = (oss_media_file_t*)malloc(sizeof(oss_media_file_t));
    file->endpoint = "https://oss.abc.com";
    file->bucket_name = "bucket-1";
    file->object_key = "key-1";
    file->is_cname = 0;
    file->mode = "w";

    char *url = oss_media_create_ts_full_url(pool, file);
    CuAssertStrEquals(tc, "https://bucket-1.oss.abc.com/key-1", url);

    aos_pool_destroy(pool);
    oss_media_file_close(file);
}

void test_oss_media_set_m3u8_info(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "dir/test.m3u8";
    option.is_live = 1;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 700;
    option.hls_list_size = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->file->endpoint = "oss.abc.com";
    stream->ts_file->file->bucket_name = "bucket-1";

    oss_media_set_m3u8_info(0, 10.5, stream);
    
    CuAssertDblEquals(tc, 10.5, stream->m3u8_infos[0].duration, 0.00001);
    CuAssertStrEquals(tc, "http://bucket-1.oss.abc.com/dir/test0.ts", 
                      stream->m3u8_infos[0].url);

    delete_file(stream->m3u8_file->file);

    ret = oss_media_ts_stream_close(stream);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_write_m3u8_for_vod(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test2-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "dir/test2.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->file->endpoint = "oss.abc.com";
    stream->ts_file->file->bucket_name = "bucket-1";

    ret = oss_media_write_m3u8(8.9, stream);
    CuAssertIntEquals(tc, 0, ret);
    
    char *content = stream->m3u8_file->buffer->buf;
    char *expected = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#"
                     "EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-VERSION:3\n"
                     "#EXTINF:8.900,\nhttp://bucket-1.oss.abc.com/dir/test2-0.ts\n";
    CuAssertStrnEquals(tc, expected, strlen(expected), content);

    ret = oss_media_write_m3u8(7.2, stream);
    CuAssertIntEquals(tc, 0, ret);

    content = stream->m3u8_file->buffer->buf;
    expected = "#EXTINF:7.200,\nhttp://bucket-1.oss.abc.com/dir/test2-0.ts\n";
    CuAssertStrnEquals(tc, expected, strlen(expected), content);

    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);    
}

void test_oss_media_write_m3u8_for_live(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test2-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "dir/test2.m3u8";
    option.is_live = 1;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    option.hls_list_size = 3;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->file->endpoint = "oss.abc.com";
    stream->ts_file->file->bucket_name = "bucket-1";

    ret = oss_media_write_m3u8(8.9, stream);
    CuAssertIntEquals(tc, 0, ret);
    
    char *content = stream->m3u8_file->buffer->buf;
    char *expected = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#"
                     "EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-VERSION:3\n"
                     "#EXTINF:8.900,\nhttp://bucket-1.oss.abc.com/dir/test2-0.ts\n";
    CuAssertStrnEquals(tc, expected, strlen(expected), content);

    close_and_open_new_file(stream);
    stream->ts_file->file->endpoint = "oss.abc.com";
    stream->ts_file->file->bucket_name = "bucket-1";
    ret = oss_media_write_m3u8(7.2, stream);
    CuAssertIntEquals(tc, 0, ret);

    content = stream->m3u8_file->buffer->buf;
    expected = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#"
               "EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-VERSION:3\n"
               "#EXTINF:8.900,\nhttp://bucket-1.oss.abc.com/dir/test2-0.ts\n"
               "#EXTINF:7.200,\nhttp://bucket-1.oss.abc.com/dir/test2-1.ts\n";

    CuAssertStrnEquals(tc, expected, strlen(expected), content);

    close_and_open_new_file(stream);
    stream->ts_file->file->endpoint = "oss.abc.com";
    stream->ts_file->file->bucket_name = "bucket-1";
    ret = oss_media_write_m3u8(5.3, stream);
    CuAssertIntEquals(tc, 0, ret);

    close_and_open_new_file(stream);
    stream->ts_file->file->endpoint = "oss.abc.com";
    stream->ts_file->file->bucket_name = "bucket-1";
    ret = oss_media_write_m3u8(5.6, stream);
    CuAssertIntEquals(tc, 0, ret);

    content = stream->m3u8_file->buffer->buf;
    expected = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#"
               "EXT-X-MEDIA-SEQUENCE:1\n#EXT-X-VERSION:3\n"
               "#EXTINF:7.200,\nhttp://bucket-1.oss.abc.com/dir/test2-1.ts\n"
               "#EXTINF:5.300,\nhttp://bucket-1.oss.abc.com/dir/test2-2.ts\n"
               "#EXTINF:5.600,\nhttp://bucket-1.oss.abc.com/dir/test2-3.ts\n";
    CuAssertStrnEquals(tc, expected, strlen(expected), content);

    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);    
}

void test_oss_media_write_m3u8_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test2-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "dir/test2.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 1;

    ret = oss_media_write_m3u8(8.9, stream);
    CuAssertTrue(tc, 0 != ret);

    stream->ts_file->file->endpoint = TEST_OSS_ENDPOINT;
    stream->ts_file->file->bucket_name = TEST_BUCKET_NAME;
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_close_and_open_new_file_with_close_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test3-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "dir/test3.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 1;

    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 0;

    ret = close_and_open_new_file(stream);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_stream_close(stream);
}

void test_close_and_open_new_file_with_open_failed(CuTest *tc) {
    char *bucket = (char*)malloc(strlen(TEST_BUCKET_NAME) + 1);
    memcpy(bucket, TEST_BUCKET_NAME, strlen(TEST_BUCKET_NAME) + 1);

    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test4-";
    option.bucket_name = bucket;
    option.m3u8_name = "dir/test4.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 0;
    memcpy(bucket, "b.1", strlen("b.1") + 1);

    ret = close_and_open_new_file(stream);
    CuAssertIntEquals(tc, -1, ret);    

    free(bucket);
    oss_media_ts_stream_close(stream);
}

void test_close_and_open_new_file(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "dir/test5-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "dir/test5.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    CuAssertStrEquals(tc, "dir/test5-0.ts", stream->ts_file->file->object_key);

    ret = close_and_open_new_file(stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertStrEquals(tc, "dir/test5-1.ts", stream->ts_file->file->object_key);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_flush_with_write_ts_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test6-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test6.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 1;

    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 0;

    ret = oss_media_ts_stream_flush(1.2, stream);
    CuAssertIntEquals(tc, -1, ret);

    delete_file(stream->m3u8_file->file);    
    oss_media_ts_stream_close(stream);    
}

void test_oss_media_ts_stream_flush_with_write_m3u8_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test6-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test7.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 0;
    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 1;

    ret = oss_media_ts_stream_flush(1.2, stream);
    CuAssertIntEquals(tc, -1, ret);

    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);    
}

void test_oss_media_ts_stream_flush(CuTest *tc) {
    char *ts_content = "abc";

    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test36-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test36.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    
    memcpy(&stream->ts_file->buffer->buf[stream->ts_file->buffer->pos],
           ts_content, strlen(ts_content));
    stream->ts_file->buffer->pos += strlen(ts_content);

    ret = oss_media_ts_stream_flush(1.2, stream);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertIntEquals(tc, stream->ts_file->buffer->start,
                      stream->ts_file->buffer->pos);
    CuAssertIntEquals(tc, stream->m3u8_file->buffer->start, 
                      stream->m3u8_file->buffer->pos);

    oss_media_ts_stream_close(stream);

    // read ts file content
    oss_media_file_t* ts_file = oss_media_file_open(TEST_BUCKET_NAME, 
            "test36-0.ts", "r", auth_func);
    CuAssertTrue(tc, ts_file != NULL);    

    uint8_t *buffer = (uint8_t*)malloc(strlen(ts_content));
    int length = oss_media_file_read(ts_file, buffer, strlen(ts_content));
    CuAssertIntEquals(tc, strlen(ts_content), length);
    CuAssertStrnEquals(tc, ts_content, strlen(ts_content), (char*)buffer);

    delete_file(ts_file);
    oss_media_file_close(ts_file);

    // read m3u8 file content
    oss_media_file_t* m3u8_file = oss_media_file_open(TEST_BUCKET_NAME, 
            option.m3u8_name, "r", auth_func);
    CuAssertTrue(tc, m3u8_file != NULL);
    free(buffer);

    int file_len = m3u8_file->_stat.length;
    buffer = (uint8_t*)malloc(file_len);
    length = oss_media_file_read(m3u8_file, buffer, file_len);
    CuAssertIntEquals(tc, file_len, length);
    CuAssertTrue(tc, length > 0);

    delete_file(m3u8_file);
    oss_media_file_close(m3u8_file);
    free(buffer);
}

void test_oss_media_need_flush_with_less_than_hls_time(CuTest *tc) {
    oss_media_ts_frame_t frame;
    frame.stream_type = st_aac;
    
    int ret = oss_media_need_flush(10, 12, &frame);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_need_flush_with_aac(CuTest *tc) {
    oss_media_ts_frame_t frame;
    frame.stream_type = st_aac;
    
    int ret = oss_media_need_flush(15, 12, &frame);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_need_flush_with_mp3(CuTest *tc) {
    oss_media_ts_frame_t frame;
    frame.stream_type = st_mp3;
    
    int ret = oss_media_need_flush(15, 12, &frame);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_need_flush_with_h264(CuTest *tc) {
    oss_media_ts_frame_t frame;
    frame.stream_type = st_h264;
    
    int ret = oss_media_need_flush(15, 12, &frame);
    CuAssertIntEquals(tc, 1, ret);
}

void test_oss_media_get_samples_per_frame_with_mp3(CuTest *tc) {
    oss_media_ts_stream_t stream;
    oss_media_ts_frame_t audio_frame;
    audio_frame.stream_type = st_mp3;
    stream.audio_frame = &audio_frame;

    int sample = oss_media_get_samples_per_frame(&stream);
    CuAssertIntEquals(tc, OSS_MEDIA_MP3_SAMPLE_RATE, sample);
}

void test_oss_media_get_samples_per_frame_with_aac(CuTest *tc) {
    oss_media_ts_stream_t stream;
    oss_media_ts_frame_t audio_frame;
    audio_frame.stream_type = st_aac;
    stream.audio_frame = &audio_frame;

    int sample = oss_media_get_samples_per_frame(&stream);
    CuAssertIntEquals(tc, OSS_MEDIA_AAC_SAMPLE_RATE, sample);
}

void test_oss_media_write_stream_frame_with_write_frame_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test7-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test7.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 10;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 1;    
    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 0;

    oss_media_ts_frame_t frame;
    frame.pts = 5000;
    frame.dts = 5000;
    frame.stream_type = st_mp3;
    
    ret = oss_media_write_stream_frame(&frame, stream);
    CuAssertIntEquals(tc, -1, ret);
    CuAssertIntEquals(tc, frame.pts, stream->current_file_begin_pts);
    
    oss_media_ts_stream_close(stream);
}

void test_oss_media_write_stream_frame_with_flush_ts_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test8-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test8.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 1;
    stream->current_file_begin_pts = 800000;

    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 0;

    oss_media_ts_frame_t frame;
    frame.pts = 5000;
    frame.dts = 5000;
    frame.stream_type = st_h264;
    
    ret = oss_media_write_stream_frame(&frame, stream);
    CuAssertIntEquals(tc, -1, ret);
    CuAssertIntEquals(tc, 800000, stream->current_file_begin_pts);

    oss_media_ts_stream_close(stream);
}

void test_oss_media_write_stream_frame_with_new_file_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test9-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test9.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 0;
    stream->current_file_begin_pts = 800000;

    oss_media_ts_frame_t frame;
    frame.pts = 5000;
    frame.dts = 5000;
    frame.stream_type = st_h264;
    
    ret = oss_media_write_stream_frame(&frame, stream);
    CuAssertIntEquals(tc, -1, ret);
    CuAssertIntEquals(tc, 800000, stream->current_file_begin_pts);

    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_write_stream_frame_succeeded(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test10-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test10.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = -5;
    stream->current_file_begin_pts = 800000;

    oss_media_ts_frame_t frame;
    frame.pts = 5000;
    frame.dts = 5000;
    frame.stream_type = st_h264;

    CuAssertStrEquals(tc, "test10-0.ts", stream->ts_file->file->object_key);
    
    ret = oss_media_write_stream_frame(&frame, stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, -1, stream->current_file_begin_pts);
    CuAssertStrEquals(tc, "test10-1.ts", stream->ts_file->file->object_key);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_get_video_frame_with_no_data(CuTest *tc) {
    oss_media_ts_stream_t stream;
    uint8_t *buf;
    
    int ret = oss_media_get_video_frame(buf, 0, &stream);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_get_video_frame_with_no_consume(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test11-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test11.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->video_frame->end = stream->video_frame->pos + 10;
    
    uint8_t *buf;
    
    ret = oss_media_get_video_frame(buf, 10, stream);
    CuAssertIntEquals(tc, 1, ret);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_get_video_frame_with_get_first_frame(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test12-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test12.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    
    uint8_t buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                     0x00, 0x00, 0x00, 0x01, 0x67};

    stream->video_frame->end = buf;
    stream->video_frame->pos = buf;
    
    ret = oss_media_get_video_frame(buf, sizeof(buf), stream);
    CuAssertIntEquals(tc, 1, ret);
    CuAssertPtrEquals(tc, buf, stream->video_frame->pos);
    CuAssertPtrEquals(tc, buf + 6, stream->video_frame->end);
    CuAssertIntEquals(tc, 5000 + 90 * option.video_frame_rate,
                      stream->video_frame->pts);
    CuAssertIntEquals(tc, stream->video_frame->dts, stream->video_frame->pts);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_get_video_frame_with_get_last_frame(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test33-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test33.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    
    uint8_t buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10};
    
    stream->video_frame->end = buf;
    stream->video_frame->pos = buf;
    
    ret = oss_media_get_video_frame(buf, sizeof(buf), stream);
    CuAssertIntEquals(tc, 1, ret);
    CuAssertPtrEquals(tc, buf, stream->video_frame->pos);
    CuAssertPtrEquals(tc, buf + 6, stream->video_frame->end);
    CuAssertIntEquals(tc, 5000 + 90 * option.video_frame_rate,
                      stream->video_frame->pts);
    CuAssertIntEquals(tc, stream->video_frame->dts, stream->video_frame->pts);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);    
}

void test_oss_media_get_audio_frame_with_no_data(CuTest *tc) {
    oss_media_ts_stream_t stream;
    uint8_t *buf;
    
    int ret = oss_media_get_audio_frame(buf, 0, &stream);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_get_audio_frame_with_no_consume(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test14-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test14.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->audio_frame->end = stream->audio_frame->pos + 10;
    
    uint8_t *buf;
    
    ret = oss_media_get_audio_frame(buf, 10, stream);
    CuAssertIntEquals(tc, 1, ret);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_get_audio_frame_with_get_first_frame(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test15-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test15.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    
    uint8_t buf[] = {0xFF, 0xF0, 0x00, 0x01, 0x09, 0x10, 
                     0xFF, 0xF1};
    
    stream->audio_frame->end = buf;
    stream->audio_frame->pos = buf;
    
    ret = oss_media_get_audio_frame(buf, sizeof(buf), stream);
    CuAssertIntEquals(tc, 1, ret);
    CuAssertPtrEquals(tc, buf, stream->audio_frame->pos);
    CuAssertPtrEquals(tc, buf + 6, stream->audio_frame->end);
    CuAssertIntEquals(tc, 5000 + (90000 * OSS_MEDIA_AAC_SAMPLE_RATE) / 
                      stream->option->audio_sample_rate,
                      stream->audio_frame->pts);
    CuAssertIntEquals(tc, stream->audio_frame->dts, stream->audio_frame->pts);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_get_audio_frame_with_get_last_frame(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test23-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test23.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);
    
    uint8_t buf[] = {0xFF, 0xF0, 0x00, 0x01, 0x09, 0x10};
    
    stream->audio_frame->end = buf;
    stream->audio_frame->pos = buf;
    
    ret = oss_media_get_audio_frame(buf, sizeof(buf), stream);
    CuAssertIntEquals(tc, 1, ret);
    CuAssertPtrEquals(tc, buf, stream->audio_frame->pos);
    CuAssertPtrEquals(tc, buf + 6, stream->audio_frame->end);
    CuAssertIntEquals(tc, 5000 + (90000 * OSS_MEDIA_AAC_SAMPLE_RATE) / 
                      stream->option->audio_sample_rate,
                      stream->audio_frame->pts);
    CuAssertIntEquals(tc, stream->audio_frame->dts, stream->audio_frame->pts);
    
    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_sync_pts_dts_with_sync_video(CuTest *tc) {
    oss_media_ts_stream_t stream;
    oss_media_ts_frame_t audio_frame;
    audio_frame.pts = 10000;
    audio_frame.dts = 10000;
    stream.audio_frame = &audio_frame;

    oss_media_ts_frame_t video_frame;
    video_frame.pts = 5000;
    video_frame.dts = 5000;
    stream.video_frame = &video_frame;

    oss_media_sync_pts_dts(&stream);
    
    CuAssertIntEquals(tc, 10000, audio_frame.pts);
    CuAssertIntEquals(tc, 10000, audio_frame.dts);
    CuAssertIntEquals(tc, 10000, video_frame.pts);
    CuAssertIntEquals(tc, 10000, video_frame.dts);
}

void test_oss_media_sync_pts_dts_with_sync_audio(CuTest *tc) {
    oss_media_ts_stream_t stream;
    oss_media_ts_frame_t audio_frame;
    audio_frame.pts = 5000;
    audio_frame.dts = 5000;
    stream.audio_frame = &audio_frame;

    oss_media_ts_frame_t video_frame;
    video_frame.pts = 10000;
    video_frame.dts = 10000;
    stream.video_frame = &video_frame;

    oss_media_sync_pts_dts(&stream);
    
    CuAssertIntEquals(tc, 10000, audio_frame.pts);
    CuAssertIntEquals(tc, 10000, audio_frame.dts);
    CuAssertIntEquals(tc, 10000, video_frame.pts);
    CuAssertIntEquals(tc, 10000, video_frame.dts);
}

void test_oss_media_ts_stream_write_with_no_video_audio(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test24-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test24.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;

    uint8_t *video_buf;
    uint8_t *audio_buf;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);    

    ret = oss_media_ts_stream_write(video_buf, 0, audio_buf, 0, stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 0, stream->ts_file->buffer->pos);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_write_with_only_video(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test43-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test43.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;

    uint8_t video_buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0xfa};
    uint8_t *audio_buf;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    ret = oss_media_ts_stream_write(video_buf, sizeof(video_buf),
                                    audio_buf, 0, stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 564, stream->ts_file->buffer->pos);

    oss_media_ts_stream_flush(10, stream);    
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_write_with_only_audio(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test16-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test16.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;

    uint8_t *video_buf;
    uint8_t audio_buf[] = {0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF1};
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    ret = oss_media_ts_stream_write(video_buf, 0,
                                    audio_buf, sizeof(audio_buf), stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 752, stream->ts_file->buffer->pos);

    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);    
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_write(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test53-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test53.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;

    uint8_t video_buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0x81, 0x1f, 0xff,
                           0x00, 0x00, 0x00, 0x01, 0x63, 0xba, 0xfa};
    uint8_t audio_buf[] = {0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad};
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    ret = oss_media_ts_stream_write(video_buf, sizeof(video_buf),
                                    audio_buf, sizeof(audio_buf), stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 1692, stream->ts_file->buffer->pos);

    oss_media_ts_stream_flush(10, stream);    
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_write_with_same_pts(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test63-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test63.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 2;
    option.audio_sample_rate = 1024 * 1000;
    option.hls_time = 5;

    uint8_t video_buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0x81, 0x1f, 0xff,
                           0x00, 0x00, 0x00, 0x01, 0x63, 0xba, 0xfa};
    uint8_t audio_buf[] = {0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad};
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    ret = oss_media_ts_stream_write(video_buf, sizeof(video_buf),
                                    audio_buf, sizeof(audio_buf), stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 1692, stream->ts_file->buffer->pos);

    oss_media_ts_stream_flush(10, stream);
    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_write_failed(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test16-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test16.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;

    uint8_t video_buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0x81, 0x1f, 0xff,
                           0x00, 0x00, 0x00, 0x01, 0x63, 0xba, 0xfa};
    uint8_t *audio_buf;
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    stream->video_frame->pts = 900000;
    stream->current_file_begin_pts = 0;
    stream->m3u8_file->options.handler_func = oss_media_ts_fake_handler;
    stream->m3u8_file->frame_count = 1;

    ret = oss_media_ts_stream_write(video_buf, sizeof(video_buf),
                                    audio_buf, 0, stream);
    CuAssertIntEquals(tc, -1, ret);

    delete_file(stream->ts_file->file);
    delete_file(stream->m3u8_file->file);    
    oss_media_ts_stream_close(stream);
}

void test_oss_media_ts_stream_close_for_vod(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test20-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test20.m3u8";
    option.is_live = 0;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;

    uint8_t video_buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0x81, 0x1f, 0xff,
                           0x00, 0x00, 0x00, 0x01, 0x63, 0xba, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0xaa, 0xcc, 0xdd};
    uint8_t audio_buf[] = {0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad};
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    ret = oss_media_ts_stream_write(video_buf, sizeof(video_buf),
                                    audio_buf, 0, stream);
    CuAssertIntEquals(tc, 0, ret);

    // read ts file content
    oss_media_file_t* ts_file = oss_media_file_open(TEST_BUCKET_NAME, 
            "test20-0.ts", "r", auth_func);
    CuAssertTrue(tc, ts_file != NULL);    

    uint8_t *buffer = (uint8_t*)malloc(18800);
    int length = oss_media_file_read(ts_file, buffer, 1880);
    CuAssertIntEquals(tc, 0, length);

    oss_media_file_close(ts_file);

    // read m3u8 file content
    oss_media_file_t* m3u8_file = oss_media_file_open(TEST_BUCKET_NAME, 
            option.m3u8_name, "r", auth_func);
    CuAssertTrue(tc, m3u8_file != NULL);
    free(buffer);

    int file_len = m3u8_file->_stat.length;
    buffer = (uint8_t*)malloc(file_len);
    length = oss_media_file_read(m3u8_file, buffer, file_len);
    CuAssertIntEquals(tc, 0, length);
    oss_media_file_close(m3u8_file);
    free(buffer);

    ret = oss_media_ts_stream_close(stream);
    CuAssertIntEquals(tc, 0, ret);

    // read ts file content
    ts_file = oss_media_file_open(TEST_BUCKET_NAME, 
                                  "test20-0.ts", "r", auth_func);
    CuAssertTrue(tc, ts_file != NULL);    

    buffer = (uint8_t*)malloc(18800);
    length = oss_media_file_read(ts_file, buffer, 1880);
    CuAssertIntEquals(tc, 1128, length);

    delete_file(ts_file);
    oss_media_file_close(ts_file);

    // read m3u8 file content
    m3u8_file = oss_media_file_open(TEST_BUCKET_NAME, 
                                    option.m3u8_name, "r", auth_func);
    CuAssertTrue(tc, m3u8_file != NULL);
    free(buffer);

    file_len = m3u8_file->_stat.length;
    buffer = (uint8_t*)malloc(file_len);
    length = oss_media_file_read(m3u8_file, buffer, file_len);
    CuAssertIntEquals(tc, 161, length);    

    delete_file(m3u8_file);
    oss_media_file_close(m3u8_file);
    free(buffer);
}

void test_oss_media_ts_stream_close_for_live(CuTest *tc) {
    oss_media_ts_stream_option_t option;
    option.ts_name_prefix = "test21-";
    option.bucket_name = TEST_BUCKET_NAME;
    option.m3u8_name = "test21.m3u8";
    option.is_live = 1;
    option.video_frame_rate = 33;
    option.audio_sample_rate = 24000;
    option.hls_time = 5;
    option.hls_list_size = 3;

    uint8_t video_buf[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0x81, 0x1f, 0xff,
                           0x00, 0x00, 0x00, 0x01, 0x63, 0xba, 0xfa,
                           0x00, 0x00, 0x00, 0x01, 0xaa, 0xcc, 0xdd};
    uint8_t audio_buf[] = {0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad,
                           0xFF, 0xF0, 0x00, 0x91, 0x9d, 0xad};
    
    int ret;
    oss_media_ts_stream_t *stream;
    stream = oss_media_ts_stream_open(auth_func, &option);
    CuAssertTrue(tc, stream != NULL);

    ret = oss_media_ts_stream_write(video_buf, sizeof(video_buf),
                                    audio_buf, 0, stream);
    CuAssertIntEquals(tc, 0, ret);

    ret = oss_media_ts_stream_close(stream);
    CuAssertIntEquals(tc, 0, ret);

    // read ts file content
    oss_media_file_t* ts_file = oss_media_file_open(TEST_BUCKET_NAME, 
            "test21-0.ts", "r", auth_func);
    CuAssertTrue(tc, ts_file != NULL);    

    uint8_t *buffer = (uint8_t*)malloc(1880);
    int length = oss_media_file_read(ts_file, buffer, 1880);
    CuAssertIntEquals(tc, 1128, length);

    delete_file(ts_file);
    oss_media_file_close(ts_file);

    // read m3u8 file content
    oss_media_file_t* m3u8_file = oss_media_file_open(TEST_BUCKET_NAME, 
            option.m3u8_name, "r", auth_func);
    CuAssertTrue(tc, m3u8_file != NULL);
    free(buffer);

    int64_t file_len = m3u8_file->_stat.length;
    buffer = (uint8_t*)malloc(file_len);
    length = oss_media_file_read(m3u8_file, buffer, file_len);
    CuAssertIntEquals(tc, 147, length);
    
    delete_file(m3u8_file);
    oss_media_file_close(m3u8_file);
    free(buffer);
}

static int oss_media_ts_fake_handler(oss_media_ts_file_t *file) {
    return file->frame_count++ > 0;
}

static void auth_func(oss_media_file_t *file) {
    file->endpoint = TEST_OSS_ENDPOINT;
    file->is_cname = 0;
    file->access_key_id = TEST_ACCESS_KEY_ID;
    file->access_key_secret = TEST_ACCESS_KEY_SECRET;
    file->token = NULL; //SAMPLE_STS_TOKEN; // if use sts token

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

CuSuite *test_ts_stream()
{
    CuSuite* suite = CuSuiteNew();   

    SUITE_ADD_TEST(suite, test_ts_stream_setup);

    SUITE_ADD_TEST(suite, test_oss_media_get_digit_num);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_open_with_vod);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_open_with_live);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_open_with_failed);
    SUITE_ADD_TEST(suite, test_oss_media_create_ts_full_url);
    SUITE_ADD_TEST(suite, test_oss_media_create_ts_full_url_with_prefix);
    SUITE_ADD_TEST(suite, test_oss_media_set_m3u8_info);
    SUITE_ADD_TEST(suite, test_oss_media_write_m3u8_for_vod);
    SUITE_ADD_TEST(suite, test_oss_media_write_m3u8_for_live);
    SUITE_ADD_TEST(suite, test_oss_media_write_m3u8_failed);
    SUITE_ADD_TEST(suite, test_close_and_open_new_file_with_close_failed);
    SUITE_ADD_TEST(suite, test_close_and_open_new_file_with_open_failed);
    SUITE_ADD_TEST(suite, test_close_and_open_new_file);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_flush_with_write_ts_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_flush_with_write_m3u8_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_flush);
    SUITE_ADD_TEST(suite, test_oss_media_need_flush_with_less_than_hls_time);
    SUITE_ADD_TEST(suite, test_oss_media_need_flush_with_aac);
    SUITE_ADD_TEST(suite, test_oss_media_need_flush_with_mp3);
    SUITE_ADD_TEST(suite, test_oss_media_need_flush_with_h264);
    SUITE_ADD_TEST(suite, test_oss_media_get_samples_per_frame_with_aac);
    SUITE_ADD_TEST(suite, test_oss_media_get_samples_per_frame_with_mp3);
    SUITE_ADD_TEST(suite, test_oss_media_write_stream_frame_with_write_frame_failed);
    SUITE_ADD_TEST(suite, test_oss_media_write_stream_frame_with_flush_ts_failed);
    SUITE_ADD_TEST(suite, test_oss_media_write_stream_frame_with_new_file_failed);
    SUITE_ADD_TEST(suite, test_oss_media_write_stream_frame_succeeded);
    SUITE_ADD_TEST(suite, test_oss_media_get_video_frame_with_no_data);
    SUITE_ADD_TEST(suite, test_oss_media_get_video_frame_with_no_consume);
    SUITE_ADD_TEST(suite, test_oss_media_get_video_frame_with_get_first_frame);
    SUITE_ADD_TEST(suite, test_oss_media_get_video_frame_with_get_last_frame);
    SUITE_ADD_TEST(suite, test_oss_media_get_audio_frame_with_no_data);
    SUITE_ADD_TEST(suite, test_oss_media_get_audio_frame_with_no_consume);
    SUITE_ADD_TEST(suite, test_oss_media_get_audio_frame_with_get_first_frame);
    SUITE_ADD_TEST(suite, test_oss_media_get_audio_frame_with_get_last_frame);
    SUITE_ADD_TEST(suite, test_oss_media_sync_pts_dts_with_sync_video);
    SUITE_ADD_TEST(suite, test_oss_media_sync_pts_dts_with_sync_audio);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_write_with_no_video_audio);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_write_with_only_video);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_write_with_only_audio);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_write);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_write_with_same_pts);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_write_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_close_for_vod);
    SUITE_ADD_TEST(suite, test_oss_media_ts_stream_close_for_live);
    
    SUITE_ADD_TEST(suite, test_ts_stream_teardown); 
    
    return suite;
}
