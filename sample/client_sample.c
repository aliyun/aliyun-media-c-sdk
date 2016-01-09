#include "config.h"
#include "src/oss_media_file.h"

char *g_filename="oss_media_file";
char *g_val = "hello oss media file, this is my first time to write content.\n";

static void oss_clean(const char *filename) {
    aos_pool_t *pool;
    oss_request_options_t *opts;
    aos_string_t bucket;
    aos_string_t key;
    aos_status_t *status;
    aos_table_t *resp_headers;

    aos_pool_create(&pool, NULL);

    opts = oss_request_options_create(pool);
    opts->config = oss_config_create(pool);
    aos_str_set(&opts->config->host, SAMPLE_OSS_HOST);
    aos_str_set(&opts->config->id, SAMPLE_ACCESS_KEY_ID);
    aos_str_set(&opts->config->key, SAMPLE_ACCESS_KEY_SECRET);
    opts->config->is_oss_domain = 1; //oss host type, host is oss domain ? 1 : 0(cname etc)
    opts->ctl = aos_http_controller_create(pool, 0);
    aos_str_set(&bucket, SAMPLE_BUCKET_NAME);
    aos_str_set(&key, filename);

    status = oss_delete_object(opts, &bucket, &key, &resp_headers);
    if (!aos_status_is_ok(status)) {
        aos_error_log("clean oss error. [code=%d, message=%s]", 
                      status->code, status->error_code);
        aos_error_log("example exit");
        aos_pool_destroy(pool);
        exit(-1);
    }

    aos_pool_destroy(pool);
}

static void auth_func(oss_media_file_t *file) {
    file->host = SAMPLE_OSS_HOST;
    file->is_oss_domain = 1; //oss host type, host is oss domain ? 1 : 0(cname etc)
    file->bucket= SAMPLE_BUCKET_NAME;
    file->filename=g_filename;
    file->access_key_id = SAMPLE_ACCESS_KEY_ID;
    file->access_key_secret = SAMPLE_ACCESS_KEY_SECRET;
    file->token = NULL; //set NULL if not use token, otherwise use SAMPLE_STS_TOKEN,

    // expiration 300 sec.
    file->expiration = time(NULL) + 300;
}

static void write_file() {
    oss_clean(g_filename);

    int64_t write_size;
    oss_media_file_t *file;

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    if (0 != oss_media_file_open(file, "w")) {
        printf("open media file[%s] failed\n", file->filename);
        oss_media_file_free(file);
        return;
    }

    // write file
    write_size = oss_media_file_write(file, g_val, strlen(g_val));
    if (-1 != write_size) {
        printf("write %" PRId64 " bytes succeeded\n", write_size);
    } else {
        oss_media_file_close(file);
        oss_media_file_free(file);

        printf("write failed\n");
        return;
    }

    write_size = oss_media_file_write(file, g_val, strlen(g_val));
    if (-1 != write_size) {
        printf("write %" PRId64 " bytes succeeded\n", write_size);
    } else {
        oss_media_file_close(file);
        oss_media_file_free(file);

        printf("write failed\n");
        return;
    }

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);

    printf("oss media c sdk write object succeeded\n");
}

static void append_file() {
    oss_clean(g_filename);

    int i;
    int64_t write_size;
    oss_media_file_t *file;

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    if(0 != oss_media_file_open(file, "a")) {
        oss_media_file_free(file);

        printf("open media file[%s] failed\n", file->filename);
        return;
    }

    // write file 
    for (i = 0; i < 8; i++) {
        write_size = oss_media_file_write(file, g_val, strlen(g_val));
        if (-1 != write_size) {
            printf("write %" PRId64 " bytes succeeded\n", write_size);
        } else {
            oss_media_file_close(file);
            oss_media_file_free(file);

            printf("write failed\n");
            return;
        } 
    }

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);

    printf("oss media c sdk append object succeeded\n");
}

static void read_file() {
    char *content;
    int ntotal, nread, nbuf=16;
    char buf[nbuf];

    oss_media_file_t *file;

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    if (0 != oss_media_file_open(file, "r")) {
        oss_media_file_free(file);

        printf("open media file[%s] failed\n", file->filename);
        return;
    }

    // stat file
    oss_media_file_stat_t stat;
    if (0 != oss_media_file_stat(file, &stat)) {
        oss_media_file_close(file);
        oss_media_file_free(file);

        printf("stat media file[%s] failed\n", file);
        return;
    }

    aos_info_log("file [name=%s, length=%" PRId64 ", type=%s]", 
                 file->filename, stat.length, stat.type);

    // read file
    content = malloc(stat.length + 1);
    ntotal = 0;
    while ((nread = oss_media_file_read(file, buf, nbuf)) > 0) {
        memcpy(content + ntotal, buf, nread);
        ntotal += nread;
    }
    content[ntotal] = '\0';
    aos_info_log("file content length is: %d", ntotal);

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);
    free(content);

    printf("oss media c sdk read object succeeded\n");
}

static void seek_file() {
    char *content;
    int ntotal, nread, nbuf=16;
    char buf[nbuf];

    oss_media_file_t *file;

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    if (0 != oss_media_file_open(file, "r")) {
        oss_media_file_free(file);

        printf("open media file[%s] failed\n", file->filename);
        return;
    }

    // stat
    oss_media_file_stat_t stat;
    if (0 != oss_media_file_stat(file, &stat)) {
        oss_media_file_close(file);
        oss_media_file_free(file);

        printf("stat media file[%s] failed\n", file);
        return;
    }

    aos_info_log("file [name=%s, length=%" PRId64 ", type=%s]", 
                 file->filename, stat.length, stat.type);

    // tell
    aos_info_log("file [position=%" PRId64 "]", oss_media_file_tell(file));

    // seek
    oss_media_file_seek(file, stat.length / 2);

    // tell
    aos_info_log("file [position=%" PRId64 "] after seek %ld", 
                 oss_media_file_tell(file), stat.length / 2);

    // read
    content = malloc(stat.length / 2 + 2);
    ntotal = 0;
    while ((nread = oss_media_file_read(file, buf, nbuf)) > 0) {
        memcpy(content + ntotal, buf, nread);
        ntotal += nread;
    }
    content[ntotal] = '\0';
    aos_info_log("file content is: \n%s", content);

    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);
    free(content);

    printf("oss media c sdk seek object succeeded\n");
}

static void error_code() {
    // prepare
    write_file();
    
    int64_t write_size;
    oss_media_file_t *file;

    // create file
    file = oss_media_file_create();
    file->auth = auth_func;

    // open file
    if (0 != oss_media_file_open(file, "a")) {
        oss_media_file_free(file);

        printf("open media file[%s] failed\n", file->filename);
        return;
    }

    // write file
    write_size = oss_media_file_write(file, g_val, strlen(g_val));
    if (-1 != write_size) {
        printf("write %" PRId64 " bytes succeeded\n", write_size);
    } else {
        printf("write [%s] failed\n", file->filename);
    } 
    
    // close file and free
    oss_media_file_close(file);
    oss_media_file_free(file);

    printf("oss media c sdk try error code  succeeded\n");
}

static void usage() {
    printf("Usage: oss_media_client_example type\n"
           "type:\n"
           "     write\n"
           "     append\n"
           "     read\n"
           "     seek\n"
           "     error_code\n");
}

int main(int argc, char *argv[]) {
    oss_media_init(AOS_LOG_INFO);
    
    if (argc < 2) {
        usage();
        oss_media_destroy();
        return -1;
    }
    
    // example of oss media file functions
    if (strcmp("write", argv[1]) == 0) {
        write_file();
    } else if (strcmp("append", argv[1]) == 0) {
        append_file();
    } else if (strcmp("read", argv[1]) == 0) {
        read_file();
    } else if (strcmp("seek", argv[1]) == 0) {
        seek_file();
    } else if (strcmp("error_code", argv[1]) ==0) {
        error_code();
    } else {
        printf("Unsupport operation:%s\n", argv[1]);
        usage();
    }

    oss_media_destroy();
    return 0;
}
