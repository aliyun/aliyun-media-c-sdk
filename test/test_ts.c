#include "CuTest.h"
#include "test.h"
#include "config.h"
#include "src/oss_media_ts.c"
#include <oss_c_sdk/aos_define.h>

static int64_t write_file(const char* content);
static void auth_func(oss_media_file_t *file);
static void delete_file(oss_media_file_t *file);
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
}

void test_oss_media_handle_file_with_not_called(CuTest *tc) {
    int ret = 0;
    uint8_t *p = malloc(21);
    uint8_t *start = p;

    oss_media_ts_file_t *file;
    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE;
    ret = oss_media_handle_file(file);
    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 0, file->frame_count);

    oss_media_ts_close(file);
}

void test_oss_media_handle_file_with_called_succeeded(CuTest *tc) {
    int ret = 0;
    uint8_t *p = malloc(21);
    uint8_t *start = p;

    oss_media_ts_file_t *file;
    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

    file->options.handler_func = oss_media_ts_fake_handler;

    file->buffer->end = file->buffer->pos + OSS_MEDIA_TS_PACKET_SIZE - 1;
    file->frame_count = 0;
    ret = oss_media_handle_file(file);

    CuAssertIntEquals(tc, 0, ret);
    CuAssertIntEquals(tc, 1, file->frame_count);

    oss_media_ts_close(file);
}

void test_oss_media_handle_file_with_called_failed(CuTest *tc) {
    int ret = 0;

    oss_media_ts_file_t *file;
    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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
    
    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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
    
    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);

    file->buffer->pos = file->buffer->start;
    ret = oss_media_ts_ossfile_handler(file);
    CuAssertIntEquals(tc, 0, ret);

    oss_media_ts_close(file);
}

void test_oss_media_ts_ossfile_handler_succeeded(CuTest *tc) {
    int ret;
    int64_t length;
    oss_media_ts_file_t *file;
    char *content = "hangzhou";

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "test.key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);
    
    memcpy(&file->buffer->buf[file->buffer->pos], content, sizeof(content));
    file->buffer->pos += sizeof(content);

    CuAssertTrue(tc, file->buffer->pos != file->buffer->start);

    ret = oss_media_ts_ossfile_handler(file);
    CuAssertIntEquals(tc, 0, ret);

    CuAssertTrue(tc, file->buffer->pos == file->buffer->start);

    uint8_t *buffer = (uint8_t*)malloc(sizeof(content));
    length = oss_media_file_read(file->file, buffer, sizeof(content));
    CuAssertIntEquals(tc, sizeof(content), length);
    CuAssertStrnEquals(tc, content, sizeof(content), (char*)buffer);

    oss_media_ts_close(file);
}

void test_oss_media_ts_ossfile_handler_failed(CuTest *tc) {
    int ret;
    int64_t length;
    oss_media_ts_file_t *file;
    char *content = "hangzhou";

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "test2.key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);
    
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

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "test2.key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);
    
    file->frame_count = 0;
    ret = oss_media_ts_need_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, 1, ret);
}

void test_oss_media_ts_need_write_pat_and_pmt_is_false(CuTest *tc) {
    int ret;
    oss_media_ts_file_t *file;

    ret = oss_media_ts_open(TEST_BUCKET_NAME, "test2.key", auth_func, &file);
    CuAssertIntEquals(tc, 0, ret);
    
    file->frame_count = 1;
    ret = oss_media_ts_need_write_pat_and_pmt(file);
    CuAssertIntEquals(tc, 0, ret);
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
    // SUITE_ADD_TEST(suite, test_oss_media_ts_ossfile_handler_succeeded);
    // SUITE_ADD_TEST(suite, test_oss_media_ts_ossfile_handler_failed);
    SUITE_ADD_TEST(suite, test_oss_media_ts_need_write_pat_and_pmt_is_true);
    SUITE_ADD_TEST(suite, test_oss_media_ts_need_write_pat_and_pmt_is_false);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ends_with_is_true);
    SUITE_ADD_TEST(suite, test_oss_media_ts_ends_with_is_false);

    SUITE_ADD_TEST(suite, test_ts_teardown); 
    
    return suite;
}
