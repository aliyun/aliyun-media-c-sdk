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
    
    int ret;
    oss_media_ts_stream_t *stream;
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);
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
    
    int ret;
    oss_media_ts_stream_t *stream;
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);
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
    
    int ret;
    oss_media_ts_stream_t *stream;
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, -1, ret);
}

void test_oss_media_create_ts_full_url(CuTest *tc) {
    aos_pool_t *pool;
    aos_pool_create(&pool, NULL);

    oss_media_file_t *file;
    //file = oss_media_file_open("bucket-1", "key-1", "w", auth_func);
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
    //file = oss_media_file_open("bucket-1", "key-1", "w", auth_func);
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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);

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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);

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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);

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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);

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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);

    stream->ts_file->options.handler_func = oss_media_ts_fake_handler;
    stream->ts_file->frame_count = 1;

    ret = close_and_open_new_file(stream);
    CuAssertIntEquals(tc, -1, ret);
    
    oss_media_ts_stream_close(stream);
}

void test_close_and_open_new_file_with_open_failed(CuTest *tc) {
    char *bucket = malloc(strlen(TEST_BUCKET_NAME) + 1);
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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);

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
    ret = oss_media_ts_stream_open(auth_func, &option, &stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertStrEquals(tc, "dir/test5-0.ts", stream->ts_file->file->object_key);

    ret = close_and_open_new_file(stream);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertStrEquals(tc, "dir/test5-1.ts", stream->ts_file->file->object_key);

    oss_media_ts_stream_close(stream);
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
    
    SUITE_ADD_TEST(suite, test_ts_stream_teardown); 
    
    return suite;
}
