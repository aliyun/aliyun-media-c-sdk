#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_ts.c"
#include <oss_c_sdk/aos_define.h>

extern void delete_file(oss_media_file_t *file);
static void auth_func(oss_media_file_t *file);
static int oss_media_ts_fake_handler(oss_media_ts_file_t *file);

void test_ts_setup(CuTest *tc) {
    aos_pool_t *p;
    aos_pool_create(&p, NULL);
    
    apr_dir_make(TEST_DIR"/data/", APR_OS_DEFAULT, p);

    aos_pool_destroy(p);
}

void test_ts_teardown(CuTest *tc) {
    aos_pool_t *p;
    aos_pool_create(&p, NULL);
    apr_dir_remove(TEST_DIR"/data/", p);
    aos_pool_destroy(p);
}

void test_calculate_crc32(CuTest *tc) {
    uint8_t *data = "xi'an-chengdu-hangzhou-beijing";

    uint32_t crc32 = calculate_crc32(data, strlen(data));
    CuAssertIntEquals(tc, -1266222823, crc32);

    crc32 = calculate_crc32(data, strlen(data));
    CuAssertIntEquals(tc, -1266222823, crc32);
}

void test_calculate_crc32_with_empty(CuTest *tc) {
    uint8_t *data = "xi'an-chengdu-hangzhou-beijing";
    uint32_t crc32 = calculate_crc32(data, 0);

    CuAssertIntEquals(tc, 0xFFFFFFFF, crc32);
}

void test_oss_media_ts_write_pcr(CuTest *tc) {
    uint8_t *p = malloc(6);
    uint64_t pcr = 2345678901;
    uint8_t *start = p;

    p = oss_media_ts_write_pcr(p, pcr);

    CuAssertIntEquals(tc, 6, p - start);
    CuAssertIntEquals(tc, 69, start[0]);
    CuAssertIntEquals(tc, 232, start[1]);
    CuAssertIntEquals(tc, 28, start[2]);
    CuAssertIntEquals(tc, 26, start[3]);
    CuAssertIntEquals(tc, 254, start[4]);
    CuAssertIntEquals(tc, 0, start[5]);

    free(start);
}

void test_oss_media_ts_write_pts(CuTest *tc) {
    uint8_t *p = malloc(5);
    uint32_t fb = 1;
    uint64_t pts = 2345678901;

    uint8_t *start = p;
    p = oss_media_ts_write_pts(p, fb, pts);

    CuAssertIntEquals(tc, 5, p - start);
    CuAssertIntEquals(tc, 21, start[0]);
    CuAssertIntEquals(tc, 47, start[1]);
    CuAssertIntEquals(tc, 65, start[2]);
    CuAssertIntEquals(tc, 112, start[3]);
    CuAssertIntEquals(tc, 107, start[4]);

    free(start);
}

void test_oss_media_ts_encrypt_packet(CuTest *tc) {
    CuAssertTrue(tc, 0);
}

void test_oss_media_ts_write_hls_header(CuTest *tc) {
    uint8_t *p = malloc(5);
    uint8_t *start = p;

    p = oss_media_ts_write_hls_header(p, 0);
    
    CuAssertIntEquals(tc, 5, p - start);
    CuAssertIntEquals(tc, 0x47, start[0]);
    CuAssertIntEquals(tc, 0x40, start[1]);
    CuAssertIntEquals(tc, 0x00, start[2]);
    CuAssertIntEquals(tc, 0x10, start[3]);
    CuAssertIntEquals(tc, 0x00, start[4]);

    free(start);
}

void test_oss_media_ts_set_crc32(CuTest *tc) {
    uint8_t *p = malloc(21);
    uint8_t *start = p;
    uint8_t *end;

    *p++ = 0x47;
    *p++ = 0x40;
    *p++ = 0x00;
    *p++ = 0x10;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0xb0;
    *p++ = 0x0d;
    *p++ = 0x00;
    *p++ = 0x01;
    *p++ = 0xc1;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x01;
    *p++ = 0xf0;
    *p++ = 0x01;

    end = p;
    p = oss_media_ts_set_crc32(start, p);
    
    CuAssertIntEquals(tc, 4, p - end);
    CuAssertIntEquals(tc, 0x2e, end[0]);
    CuAssertIntEquals(tc, 0x70, end[1]);
    CuAssertIntEquals(tc, 0x19, end[2]);
    CuAssertIntEquals(tc, 0x05, end[3]);
    
    free(start);
}

void test_oss_media_handle_file_with_not_called(CuTest *tc) {
    int ret = 0;
    uint8_t *p = malloc(21);
    uint8_t *start = p;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE;
    ret = oss_media_handle_file(file);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 0, file->frame_count);

    oss_media_ts_close(file);
    free(start);
}

void test_oss_media_handle_file_with_called_succeeded(CuTest *tc) {
    int ret = 0;
    uint8_t *p = malloc(21);
    uint8_t *start = p;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 0;
    ret = oss_media_handle_file(file);

    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 1, file->frame_count);

    oss_media_ts_close(file);
    free(start);
}

void test_oss_media_handle_file_with_called_failed(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 1;
    ret = oss_media_handle_file(file);
    CuAssertIntEquals(tc, -1, ret);
    CuAssertIntEquals(tc, 2, file->frame_count);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pat_with_failed(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 1;
    ret = oss_media_ts_write_pat(file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pat_with_succeeded(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 0;
    ret = oss_media_ts_write_pat(file);
    CuAssertIntEquals(tc, 0, ret);

    uint8_t *p = file->buffer->buf;
    uint8_t *start = p;

    CuAssertIntEquals(tc, 0x47, *p++);
    CuAssertIntEquals(tc, 0x40, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x10, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0xb0, *p++);
    CuAssertIntEquals(tc, 0x0d, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x01, *p++);
    CuAssertIntEquals(tc, 0xc1, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x01, *p++);
    CuAssertIntEquals(tc, 0xf0, *p++);
    CuAssertIntEquals(tc, 0x01, *p++);
    CuAssertIntEquals(tc, 0x2e, *p++);
    CuAssertIntEquals(tc, 0x70, *p++);
    CuAssertIntEquals(tc, 0x19, *p++);
    CuAssertIntEquals(tc, 0x05, *p++);
    
    while (p < start + OSS_MEDIA_TS_PACKET_SIZE) {
        CuAssertIntEquals(tc, 0xff, *p++);
    }

    CuAssertIntEquals(tc, file->buffer->start + OSS_MEDIA_TS_PACKET_SIZE,
                      file->buffer->pos);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pmt_with_failed(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 1;
    ret = oss_media_ts_write_pmt(file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pmt_with_succeeded(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 0;
    ret = oss_media_ts_write_pmt(file);
    CuAssertIntEquals(tc, 0, ret);

    uint8_t *p = file->buffer->buf;
    uint8_t *start = p;

    CuAssertIntEquals(tc, 0x47, *p++);
    CuAssertIntEquals(tc, 0x50, *p++);
    CuAssertIntEquals(tc, 0x01, *p++);
    CuAssertIntEquals(tc, 0x10, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x02, *p++);
    CuAssertIntEquals(tc, 0xb0, *p++);
    CuAssertIntEquals(tc, 0x17, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x01, *p++);
    CuAssertIntEquals(tc, 0xc1, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0xe1, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0xf0, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x1b, *p++);
    CuAssertIntEquals(tc, 0xe1, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0xf0, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x0f, *p++);
    CuAssertIntEquals(tc, 0xe1, *p++);
    CuAssertIntEquals(tc, 0x01, *p++);
    CuAssertIntEquals(tc, 0xf0, *p++);
    CuAssertIntEquals(tc, 0x00, *p++);
    CuAssertIntEquals(tc, 0x2f, *p++);
    CuAssertIntEquals(tc, 0x44, *p++);
    CuAssertIntEquals(tc, 0xb9, *p++);
    CuAssertIntEquals(tc, 0x9b, *p++);
    
    while (p < start + OSS_MEDIA_TS_PACKET_SIZE) {
        CuAssertIntEquals(tc, 0xff, *p++);
    }

    CuAssertIntEquals(tc, file->buffer->start + OSS_MEDIA_TS_PACKET_SIZE,
                      file->buffer->pos);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pat_and_pmt_with_pat_failed(CuTest *tc) {
    /*
     * pat failed since oss_media_ts_fake_handler failed
     */ 
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 1;
    ret = oss_media_ts_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pat_and_pmt_with_pmt_failed(CuTest *tc) {
    /*
     * pat succeeded since buffer is not full
     * pmt failed since oss_media_ts_fake_handler failed
     */ 
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE + 1;
    file->frame_count = 1;
    ret = oss_media_ts_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pat_and_pmt_succeeded(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = -1;
    ret = oss_media_ts_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, 0, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_pat_and_pmt_with_encrypt(CuTest *tc) {
    CuAssertTrue(tc, 0);
}

void test_oss_media_ts_ossfile_handler_without_handle(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->buffer->pos = file->buffer->start;
    ret = oss_media_ts_ossfile_handler(file);
    CuAssertIntEquals(tc, 0, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_ossfile_handler_succeeded(CuTest *tc) {
    int ret;
    int64_t length;
    oss_media_ts_file_t *file;
    char *key = "test.txt";
    char *content = "hangzhou";

    file = oss_media_ts_open(TEST_BUCKET_NAME, key, auth_func);
    CuAssertTrue(tc, file != NULL);
    
    memcpy(&file->buffer->buf[file->buffer->pos], content, strlen(content));
    file->buffer->pos += strlen(content);

    CuAssertTrue(tc, file->buffer->pos != file->buffer->start);

    ret = oss_media_ts_ossfile_handler(file);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertTrue(tc, file->buffer->pos == file->buffer->start);

    oss_media_ts_close(file);

    oss_media_file_t* read_file = oss_media_file_open(TEST_BUCKET_NAME, 
            key, "r", auth_func);
    CuAssertTrue(tc, read_file != NULL);

    uint8_t *buffer = (uint8_t*)malloc(strlen(content));
    length = oss_media_file_read(read_file, buffer, strlen(content));
    CuAssertIntEquals(tc, strlen(content), length);
    CuAssertStrnEquals(tc, content, strlen(content), (char*)buffer);

    free(buffer);
    delete_file(read_file);
    oss_media_file_close(read_file);
}

void test_oss_media_ts_ossfile_handler_failed(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;
    char *content = "hangzhou";

    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.txt", auth_func);
    CuAssertTrue(tc, file != NULL);
    
    memcpy(&file->buffer->buf[file->buffer->pos], content, sizeof(content));
    file->buffer->pos += sizeof(content);

    CuAssertTrue(tc, file->buffer->pos != file->buffer->start);

    file->file->_stat.length = 10;
    ret = oss_media_ts_ossfile_handler(file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_need_write_pat_and_pmt_is_true(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.key", auth_func);
    CuAssertTrue(tc, file != NULL);
    
    file->frame_count = 0;
    ret = oss_media_ts_need_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, 1, ret);
    
    oss_media_ts_close(file);
}

void test_oss_media_ts_need_write_pat_and_pmt_is_false(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.key", auth_func);
    CuAssertTrue(tc, file != NULL);
    
    file->frame_count = 1;
    ret = oss_media_ts_need_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, 0, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_ends_with_is_true(CuTest *tc) {
    int ret = oss_media_ts_ends_with("1.ts", ".ts");
    CuAssertIntEquals(tc, 1, ret);

    ret = oss_media_ts_ends_with(".ts", ".ts");
    CuAssertIntEquals(tc, 1, ret);
}

void test_oss_media_ts_ends_with_is_false(CuTest *tc) {
    int ret = oss_media_ts_ends_with("1.ts", "2.ts");
    CuAssertIntEquals(tc, 0, ret);

    ret = oss_media_ts_ends_with("1.ts", ".t");
    CuAssertIntEquals(tc, 0, ret);

    ret = oss_media_ts_ends_with("1.ts", "1.");
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_ts_open_with_ts_file(CuTest *tc) {
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.ts", auth_func);
    CuAssertTrue(tc, file != NULL);
    CuAssertIntEquals(tc, OSS_MEDIA_DEFAULT_WRITE_BUFFER,
                      file->buffer->end - file->buffer->start);
    CuAssertIntEquals(tc, 0, file->frame_count);
    CuAssertIntEquals(tc, OSS_MEDIA_DEFAULT_VIDEO_PID, file->options.video_pid);
    CuAssertIntEquals(tc, OSS_MEDIA_DEFAULT_AUDIO_PID, file->options.audio_pid);
    CuAssertIntEquals(tc, OSS_MEDIA_TS_HLS_DELAY, file->options.hls_delay_ms);
    CuAssertIntEquals(tc, 0, file->options.encrypt);
    CuAssertPtrEquals(tc, oss_media_ts_ossfile_handler, file->options.handler_func);
    CuAssertIntEquals(tc, OSS_MEDIA_PAT_INTERVAL_FRAME_COUNT,
                      file->options.pat_interval_frame_count);
    CuAssertIntEquals(tc, 0, file->buffer->start);
    CuAssertIntEquals(tc, 0, file->buffer->pos);

    oss_media_ts_close(file);
}

void test_oss_media_ts_open_with_m3u8_file(CuTest *tc) {
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.m3u8", auth_func);
    CuAssertTrue(tc, file != NULL);
    CuAssertIntEquals(tc, OSS_MEDIA_DEFAULT_WRITE_BUFFER / 32,
                      file->buffer->end - file->buffer->start);

    oss_media_ts_close(file);
}

void test_oss_media_ts_open_failed(CuTest *tc) {
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open("not.bucket", "test2.m3u8", auth_func);
    CuAssertTrue(tc, file == NULL);
}

void test_oss_media_ts_begin_m3u8(CuTest *tc) {
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.m3u8", auth_func);
    CuAssertTrue(tc, file != NULL);

    oss_media_ts_begin_m3u8(10, 2, file);

    char *expected = "#EXTM3U\n#EXT-X-TARGETDURATION:10\n"
                     "#EXT-X-MEDIA-SEQUENCE:2\n#EXT-X-VERSION:3\n";
    char *result = &file->buffer->buf[file->buffer->start];
    CuAssertStrnEquals(tc, expected, strlen(expected), result);

    oss_media_ts_close(file);
}

void test_oss_media_ts_end_m3u8(CuTest *tc) {
    oss_media_ts_file_t *file;
    
    file = oss_media_ts_open(TEST_BUCKET_NAME, "test2.m3u8", auth_func);
    CuAssertTrue(tc, file != NULL);

    oss_media_ts_end_m3u8(file);

    char *expected = "#EXT-X-ENDLIST";
    char *result = &file->buffer->buf[file->buffer->start];
    CuAssertStrnEquals(tc, expected, strlen(expected), result);

    oss_media_ts_flush(file);
    delete_file(file->file);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_m3u8_failed(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 1;

    oss_media_ts_m3u8_info_t m3u8[1];
    m3u8[0].duration = 0;
    memcpy(m3u8[0].url, "1.ts", strlen("1.ts"));
    ret = oss_media_ts_write_m3u8(1, m3u8, file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_m3u8_with_one_m3u8(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;
    
    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 0;

    oss_media_ts_m3u8_info_t m3u8[1];
    m3u8[0].duration = 10;
    memcpy(m3u8[0].url, "1.ts", strlen("1.ts") + 1);
    ret = oss_media_ts_write_m3u8(1, m3u8, file);
    CuAssertIntEquals(tc, 0, ret);

    char *expected = "#EXTINF:10.000,\n1.ts\n";
    char *result = &file->buffer->buf[file->buffer->start];
    CuAssertStrnEquals(tc, expected, strlen(expected), result);

    oss_media_ts_close(file);
}

void test_oss_media_ts_write_m3u8_with_two_m3u8(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;
    
    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 0;

    oss_media_ts_m3u8_info_t m3u8[2];
    m3u8[0].duration = 10;
    memcpy(m3u8[0].url, "1.ts", strlen("1.ts") + 1);
    m3u8[1].duration = 8.4;
    memcpy(m3u8[1].url, "2.ts", strlen("2.ts") + 1);
    ret = oss_media_ts_write_m3u8(2, m3u8, file);
    CuAssertIntEquals(tc, 0, ret);
    
    char *expected = "#EXTINF:10.000,\n1.ts\n#EXTINF:8.400,\n2.ts\n";
    char *result = &file->buffer->buf[file->buffer->start];
    CuAssertStrnEquals(tc, expected, strlen(expected), result);

    oss_media_ts_close(file);
}

void test_oss_media_ts_close_failed(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->frame_count = 1;
    
    ret = oss_media_ts_close(file);
    CuAssertIntEquals(tc, -1, ret);
}

void test_oss_media_ts_close_succeeded(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key1.ts", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->frame_count = 0;
    
    ret = oss_media_ts_close(file);
    CuAssertIntEquals(tc, 0, ret);
}

void test_oss_media_ts_write_frame_with_unsupport_stream_type(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key2.ts", auth_func);
    CuAssertTrue(tc, file != NULL);

    oss_media_ts_frame_t frame;
    frame.stream_type = st_mp3;
    
    ret = oss_media_ts_write_frame(&frame, file);
    CuAssertIntEquals(tc, -1, ret);

    oss_media_ts_flush(file);
    delete_file(file->file);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_frame_with_handle_file_failed(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key3.ts", auth_func);
    CuAssertTrue(tc, file != NULL);

    uint8_t *buf = (uint8_t*)malloc(1024);

    oss_media_ts_frame_t frame;
    frame.stream_type = st_aac;
    frame.pos = buf;
    frame.end = buf + 100;
    file->options.handler_func = oss_media_ts_fake_handler;
    
    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 1;
    
    ret = oss_media_ts_write_frame(&frame, file);
    CuAssertIntEquals(tc, -1, ret);

    free(buf);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_frame_with_h264(CuTest *tc) {
    /*
     * check point: 1, h.264 data. 2,with pat and pmt. 3, with adaptation
     */

    int i;
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key4.ts", auth_func);
    CuAssertTrue(tc, file != NULL);

    uint8_t buf[] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xee, 0x38, 0x30};

    oss_media_ts_frame_t frame;
    frame.stream_type = st_h264;
    frame.pos = buf;
    frame.end = buf + sizeof(buf);
    frame.pts = 5000;
    frame.dts = 5000;
    frame.continuity_counter = 7;
    frame.key = 1;
    
    ret = oss_media_ts_write_frame(&frame, file);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, OSS_MEDIA_TS_PACKET_SIZE * 3,file->buffer->pos);
    
    // check pat
    uint8_t expected[] = {0x47,0x40,0x00,0x10,0x00,0x00,0xb0,0x0d,
                          0x00,0x01,0xc1,0x00,0x00,0x00,0x01,0xf0,
                          0x01,0x2e,0x70,0x19,0x05};
    for (i = 0; i < sizeof(expected); i++) {
        CuAssertIntEquals(tc, expected[i], file->buffer->buf[i]);
    }
    for (i = sizeof(expected); i < OSS_MEDIA_TS_PACKET_SIZE; i++) {
        CuAssertIntEquals(tc, 0xff, file->buffer->buf[i]);
    }

    // check pmt
    uint8_t expected_pmt[] = {0x47,0x50,0x01,0x10,0x00,0x02,0xb0,0x17,
                              0x00,0x01,0xc1,0x00,0x00,0xe1,0x00,0xf0,
                              0x00,0x1b,0xe1,0x00,0xf0,0x00,0x0f,0xe1,
                              0x01,0xf0,0x00,0x2f,0x44,0xb9,0x9b};
    for (i = 0; i < sizeof(expected_pmt); i++) 
    {
        CuAssertIntEquals(tc, expected_pmt[i],
                          file->buffer->buf[OSS_MEDIA_TS_PACKET_SIZE + i]);
    }
    for (i = sizeof(expected_pmt); 
         i < OSS_MEDIA_TS_PACKET_SIZE; i++) 
    {
        CuAssertIntEquals(tc, 0xff, 
                          file->buffer->buf[OSS_MEDIA_TS_PACKET_SIZE + i]);
    }

    // check frame content
    uint8_t expected_frame_begin[] = {0x47,0x41,0x00,0x37,0x9c,0x50,0xff,
                                      0xff,0x8e,0xb8,0x7e,0x00};
    uint8_t expected_frame_end[] = {0x00,0x00,0x01,0xe0,0x00,0x15,0x80,0xc0,
                                    0x0a,0x31,0x00,0x05,0x13,0x41,0x11,0x00,
                                    0x05,0x13,0x41,0x00,0x00,0x00,0x01,0x68,
                                    0xee,0x38,0x30};

    for (i = 0; i < sizeof(expected_frame_begin); i++) 
    {
        CuAssertIntEquals(tc, expected_frame_begin[i], 
                          file->buffer->buf[2 * OSS_MEDIA_TS_PACKET_SIZE + i]);
    }
    for (i = 2 * OSS_MEDIA_TS_PACKET_SIZE + sizeof(expected_frame_begin); 
         i < 3 * OSS_MEDIA_TS_PACKET_SIZE - sizeof(expected_frame_end); i++) 
    {
        CuAssertIntEquals(tc, 0xff, file->buffer->buf[i]);
    }
    for (i = 0; i < sizeof(expected_frame_end); i++) 
    {
        int32_t end_pos = 3 * OSS_MEDIA_TS_PACKET_SIZE - 
                          sizeof(expected_frame_end) + i;
        CuAssertIntEquals(tc, expected_frame_end[i], file->buffer->buf[end_pos]);
    }

    CuAssertIntEquals(tc, 1, file->frame_count);

    oss_media_ts_flush(file);
    delete_file(file->file);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_frame_with_aac(CuTest *tc) {
    /*
     * check point: 1, aac data. 2,without pat and pmt. 3,without adaptation
     */
    int i;
    int ret = 0;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key5.ts", auth_func);
    CuAssertTrue(tc, file != NULL);

    file->frame_count = 1; // disable and pat/pmt table

    uint8_t buf[] = {0xff, 0xf1, 0x6c, 0x80, 0x1f, 0xa1, 0xf0};

    oss_media_ts_frame_t frame;
    frame.stream_type = st_aac;
    frame.pos = buf;
    frame.end = buf + sizeof(buf);
    frame.pts = 5000;
    frame.dts = 5000;
    frame.continuity_counter = 7;
    frame.key = 0;
    
    ret = oss_media_ts_write_frame(&frame, file);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, OSS_MEDIA_TS_PACKET_SIZE, file->buffer->pos); 
   
    // check pat
    uint8_t expected_frame_begin[] = {0x47,0x41,0x01,0x37,0xa2,0x00};
    uint8_t expected_frame_end[] = {0x00,0x00,0x01,0xc0,0x00,0x0f,0x80,
                                    0x80,0x05,0x21,0x00,0x05,0x13,0x41,
                                    0xff,0xf1,0x6c,0x80,0x1f,0xa1,0xf0};

    for (i = 0; i < sizeof(expected_frame_begin); i++) 
    {
        CuAssertIntEquals(tc, expected_frame_begin[i], file->buffer->buf[ + i]);
    }
    for (i = sizeof(expected_frame_begin); 
         i < OSS_MEDIA_TS_PACKET_SIZE - sizeof(expected_frame_end); i++) 
    {
        CuAssertIntEquals(tc, 0xff, file->buffer->buf[i]);
    }
    for (i = 0; i < sizeof(expected_frame_end); i++) 
    {
        int32_t end_pos = OSS_MEDIA_TS_PACKET_SIZE - 
                          sizeof(expected_frame_end) + i;
        CuAssertIntEquals(tc, expected_frame_end[i], file->buffer->buf[end_pos]);
    }

    CuAssertIntEquals(tc, 2, file->frame_count);

    oss_media_ts_flush(file);
    delete_file(file->file);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_frame_with_pes_overflow(CuTest *tc) {
    int i;
    int ret = 0;
    int data_len = 0xffff + 1;

    oss_media_ts_file_t *file;
    file = oss_media_ts_open(TEST_BUCKET_NAME, "key6.ts", auth_func);
    CuAssertTrue(tc, file != NULL);

    uint8_t buf[data_len];
    for (i = 0; i < data_len; i++) {
        buf[i] = i & 0xFF;
    }

    oss_media_ts_frame_t frame;
    frame.stream_type = st_h264;
    frame.pos = buf;
    frame.end = buf + sizeof(buf);
    frame.pts = 5000;
    frame.dts = 5000;
    frame.continuity_counter = 7;
    frame.key = 1;
    
    ret = oss_media_ts_write_frame(&frame, file);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, OSS_MEDIA_TS_PACKET_SIZE * 359,file->buffer->pos);

    // check frame first ts content
    uint8_t expected_frame[] = {0x47,0x41,0x00,0x37,0x07,0x50,0xff,0xff,0x8e,
                                0xb8,0x7e,0x00,0x00,0x00,0x01,0xe0,0x00,0x00,
                                0x80,0xc0,0x0a,0x31,0x00,0x05,0x13,0x41,0x11,
                                0x00,0x05,0x13,0x41,0x00,0x01,0x02,0x03,0x04,
                                0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,
                                0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
                                0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
                                0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
                                0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,
                                0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,
                                0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,
                                0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,
                                0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,
                                0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,
                                0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
                                0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,
                                0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,
                                0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,
                                0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,
                                0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,
                                0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c};

    for (i = 0; i < sizeof(expected_frame); i++) 
    {
        CuAssertIntEquals(tc, expected_frame[i], 
                          file->buffer->buf[2 * OSS_MEDIA_TS_PACKET_SIZE + i]);
    }

    CuAssertIntEquals(tc, 1, file->frame_count);

    oss_media_ts_flush(file);
    delete_file(file->file);
    oss_media_ts_close(file);
}

void test_oss_media_ts_write_frame_with_encrypt(CuTest *tc) {
    CuAssertTrue(tc, 0);
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

CuSuite *test_ts()
{
    CuSuite* suite = CuSuiteNew();   

    SUITE_ADD_TEST(suite, test_ts_setup);
    
    SUITE_ADD_TEST(suite, test_calculate_crc32);
    SUITE_ADD_TEST(suite, test_calculate_crc32_with_empty);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pcr);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pts);
    //SUITE_ADD_TEST(suite, test_oss_media_ts_encrypt_packet);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_hls_header);
    SUITE_ADD_TEST(suite, test_oss_media_ts_set_crc32);
    SUITE_ADD_TEST(suite, test_oss_media_handle_file_with_not_called);
    SUITE_ADD_TEST(suite, test_oss_media_handle_file_with_called_succeeded);
    SUITE_ADD_TEST(suite, test_oss_media_handle_file_with_called_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pat_with_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pat_with_succeeded);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pmt_with_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pmt_with_succeeded);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pat_and_pmt_with_pat_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pat_and_pmt_with_pmt_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_pat_and_pmt_succeeded);
    //SUITE_ADD_TEST(suite, test_oss_media_ts_write_pat_and_pmt_with_encrypt);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ossfile_handler_without_handle);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ossfile_handler_succeeded);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ossfile_handler_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_need_write_pat_and_pmt_is_true);
    SUITE_ADD_TEST(suite, test_oss_media_ts_need_write_pat_and_pmt_is_false);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ends_with_is_true);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ends_with_is_false);
    SUITE_ADD_TEST(suite, test_oss_media_ts_open_with_ts_file);
    SUITE_ADD_TEST(suite, test_oss_media_ts_open_with_m3u8_file);
    SUITE_ADD_TEST(suite, test_oss_media_ts_open_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_begin_m3u8);
    SUITE_ADD_TEST(suite, test_oss_media_ts_end_m3u8);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_m3u8_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_m3u8_with_one_m3u8);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_m3u8_with_two_m3u8);
    SUITE_ADD_TEST(suite, test_oss_media_ts_close_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_close_succeeded);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_frame_with_unsupport_stream_type);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_frame_with_handle_file_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_frame_with_h264);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_frame_with_aac);
    SUITE_ADD_TEST(suite, test_oss_media_ts_write_frame_with_pes_overflow);
    //SUITE_ADD_TEST(suite, test_oss_media_ts_write_frame_with_encrypt);
    
    SUITE_ADD_TEST(suite, test_ts_teardown); 
    
    return suite;
}
