#include "oss_media_file.h"

extern void oss_op_debug(char *op, 
                         aos_status_t *status, 
                         aos_table_t *req_headers, 
                         aos_table_t *resp_headers);

extern void oss_op_error(char *op, aos_status_t *status);

static void oss_init_request_opts(aos_pool_t *pool, 
                                  oss_media_file_t *file, 
                                  oss_request_options_t **options) 
{
    oss_request_options_t *opts;

    opts = oss_request_options_create(pool);
    opts->config = oss_config_create(pool);
    aos_str_set(&opts->config->host, file->host);
    aos_str_set(&opts->config->id, file->access_key_id);
    aos_str_set(&opts->config->key, file->access_key_secret);
    if (file->token)
        aos_str_set(&opts->config->sts_token, file->token);
    opts->config->is_oss_domain = file->is_oss_domain;
    opts->ctl = aos_http_controller_create(pool, 0);

    *options = opts;
}

static int64_t oss_get_content_length(const char *val) 
{
    return val ? atoll(val) : -1;
}

static void oss_auth(oss_media_file_t *file, 
                     int force) 
{
    if (!file || !(file->auth))
        return;

    // get authorize info from media server when force
    if (force) {
        file->auth(file);
    }
    // get authorize info from media server when expired
    else {
        time_t now = time(NULL);
        if (!file->expiration || now >= file->expiration) {
            file->auth(file);
        }
    }
}

static int is_readable(oss_media_file_t *file) {
    return (NULL != file->mode && 0 == strcmp("r", file->mode));
}

int oss_media_init() {
    aos_log_set_level(AOS_LOG_INFO);
    aos_log_set_output(NULL);
    return aos_http_io_initialize(OSS_MEDIA_USER_AGENT, 0);
}

void oss_media_destroy() {
    aos_http_io_deinitialize();
}

oss_media_file_t *oss_media_file_create(aos_pool_t *p) 
{
    oss_media_file_t *file = malloc(sizeof(oss_media_file_t));
    file->token = NULL;
    return file;
}

void oss_media_file_free(oss_media_file_t *file) {
    if (NULL != file) {
        free(file);
        file = NULL;
    }
}

int oss_media_file_open(oss_media_file_t *file, char *mode)
{
    if (!(strcmp("r", mode) == 0 || strcmp("w", mode) == 0 
          || strcmp("a", mode) == 0)) 
    {
        return -1;
    }
    
    //get authorize info from media server first
    oss_auth(file, 1);
    
    file->mode = mode;
    file->_read_offset = 0;

    oss_media_file_stat_t stat;
    if (oss_media_file_stat(file, &stat) == -1)
        return -1;
    file->_length = stat.length;
    file->_type = stat.type;
    return 0;
}

int oss_media_file_close(oss_media_file_t *file) {
    file->mode = "";
    file->_read_offset = 0;
    file->_length = 0;
    file->_type = "";
    return 0;
}

int oss_media_file_stat(oss_media_file_t *file, oss_media_file_stat_t *stat)
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_string_t key;
    aos_status_t *status = NULL;
    aos_table_t *req_headers = NULL;
    aos_table_t *resp_headers = NULL;

    oss_auth(file, 0);

    aos_pool_create(&pool, NULL);
    oss_init_request_opts(pool, file, &opts);
    req_headers = aos_table_make(pool, 0);
    aos_str_set(&bucket, file->bucket);
    aos_str_set(&key, file->filename);

    status = oss_head_object(opts, &bucket, &key, req_headers, &resp_headers);
    oss_op_debug("oss_head_object", status, req_headers, resp_headers);

    if (aos_status_is_ok(status)) {
        if (stat) {
            stat->length = oss_get_content_length(
                    apr_table_get(resp_headers, "Content-Length"));
            stat->type = (char*)apr_table_get(resp_headers, "x-oss-object-type");
        }
        aos_pool_destroy(pool);
        return 0;
    } else if (status->code == OSS_MEDIA_FILE_NOT_FOUND) {
        if (stat) {
            stat->length = 0;
            stat->type = OSS_MEDIA_FILE_UNKNOWN_TYPE;
        }
        aos_pool_destroy(pool);
        return 0;
    }
    aos_pool_destroy(pool);
    return -1;
}

int64_t oss_media_file_tell(oss_media_file_t *file) 
{
    if (!is_readable(file)) {
        return -1;
    }
    return file->_read_offset;
}

int64_t oss_media_file_seek(oss_media_file_t *file, int64_t offset) {
    if (!is_readable(file)) {
        return -1;
    }
    if (offset < 0 || offset >= file->_length) {
        return -1;
    }
    file->_read_offset = offset;
    return offset;
}

int64_t oss_media_file_read(oss_media_file_t *file, void *buf, int64_t nbyte)
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_string_t key;
    aos_status_t *status = NULL;
    aos_table_t *req_headers = NULL;
    aos_table_t *resp_headers = NULL;
    aos_list_t buffer;
    aos_buf_t *content = NULL;
    char *range = NULL;
    int64_t end;
    int64_t offset = 0;
    int64_t size = 0;
    int64_t len = 0;
    
    oss_auth(file, 0);

    if (!is_readable(file)) {
        return -1;
    }
    // EOF
    if (file->_read_offset > file->_length - 1) {
        return 0;
    }

    aos_pool_create(&pool, NULL);
    oss_init_request_opts(pool, file, &opts);
    req_headers = aos_table_make(pool, 0);
    aos_str_set(&bucket, file->bucket);
    aos_str_set(&key, file->filename);
    aos_list_init(&buffer);

    end = (file->_length > 0 && file->_read_offset + nbyte > file->_length) ? 
          file->_length - 1 : file->_read_offset + nbyte - 1;
    range = apr_psprintf(pool, "bytes=%" PRId64 "-%" PRId64, 
                         file->_read_offset, end);
    apr_table_set(req_headers, "Range", range);

    status = oss_get_object_to_buffer(opts, &bucket, &key, 
            req_headers, &buffer, &resp_headers);

    oss_op_debug("oss_get_object_to_buffer", status, req_headers, resp_headers);

    if (!status || !aos_status_is_ok(status)) {
        oss_op_error("oss_get_object_to_buffer", status);
        aos_pool_destroy(pool);
        return -1;
    }

    aos_list_for_each_entry(content, &buffer, node) {
        size = aos_buf_size(content);
        memcpy(buf + offset, content->pos, size);
        offset += size;
    }

    len = oss_get_content_length(apr_table_get(resp_headers, "Content-Length"));
    file->_read_offset += len;

    aos_pool_destroy(pool);
    return len;
}

int64_t oss_media_file_write(oss_media_file_t *file, const void *buf, int64_t nbyte)
{
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_string_t key;
    aos_status_t *status = NULL;
    aos_table_t *req_headers = NULL;
    aos_table_t *resp_headers = NULL;
    aos_list_t buffer;
    aos_buf_t *content = NULL;

    oss_auth(file, 0);

    if (!file->mode || (strcmp("w", file->mode) != 0 && 
                        strcmp("a", file->mode) != 0)) 
    {
        return -1;
    }

    aos_pool_create(&pool, NULL);
    oss_init_request_opts(pool, file, &opts);
    req_headers = aos_table_make(pool, 0);
    aos_str_set(&bucket, file->bucket);
    aos_str_set(&key, file->filename);
    aos_list_init(&buffer);
    content = aos_buf_pack(pool, buf, nbyte);
    aos_list_add_tail(&content->node, &buffer);

    if (strcmp("w", file->mode) == 0) {
        status = oss_put_object_from_buffer(opts, &bucket, &key, 
                &buffer, req_headers, &resp_headers);
        oss_op_debug("oss_put_object_from_buffer", status, 
                     req_headers, resp_headers);

        if (!status || !aos_status_is_ok(status)) {
            oss_op_error("oss_put_object_from_buffer", status);
            aos_pool_destroy(pool);
            return -1;
        }
    } else {
        status = oss_append_object_from_buffer(opts, &bucket, &key, 
                file->_length, &buffer, req_headers, &resp_headers);

        oss_op_debug("oss_append_object_from_buffer", status, 
                     req_headers, resp_headers);

        if (!aos_status_is_ok(status)) {
            oss_op_error("oss_append_object_from_buffer", status);
            aos_pool_destroy(pool);
            return -1;
        }
        file->_length = oss_get_content_length(apr_table_get(resp_headers, 
                        "x-oss-next-append-position"));
    }

    aos_pool_destroy(pool);
    return nbyte;
}

