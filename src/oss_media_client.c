#include "oss_media_client.h"

extern void oss_op_debug(char *op, 
                         aos_status_t *status, 
                         aos_table_t *req_headers, 
                         aos_table_t *resp_headers);

static void oss_init_request_opts(aos_pool_t *pool, 
                                  oss_media_file_t *file, 
                                  oss_request_options_t **options) 
{
    oss_request_options_t *opts;

    opts = oss_request_options_create(pool);
    opts->config = oss_config_create(pool);
    aos_str_set(&opts->config->endpoint, file->endpoint);
    aos_str_set(&opts->config->access_key_id, file->access_key_id);
    aos_str_set(&opts->config->access_key_secret, file->access_key_secret);
    if (file->token)
        aos_str_set(&opts->config->sts_token, file->token);
    opts->config->is_cname = file->is_cname;
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
    if (!file || !(file->auth_func)) {
        aos_error_log("file is null or file->auth_func is null\n");
        return;
    }

    // get authorize info from media server when force
    if (force) {
        file->auth_func(file);
    }
    // get authorize info from media server when expired
    else {
        time_t now = time(NULL);
        if (!file->expiration || now >= file->expiration) {
            file->auth_func(file);
        }
    }
}

static int is_readable(oss_media_file_t *file) {
    return (NULL != file->mode && 0 == strcmp("r", file->mode));
}

int oss_media_init(aos_log_level_e log_level) {
    aos_log_set_level(log_level);
    aos_log_set_output(NULL);
    return aos_http_io_initialize(OSS_MEDIA_CLIENT_USER_AGENT, 0);
}

void oss_media_destroy() {
    aos_http_io_deinitialize();
}

oss_media_file_t* oss_media_file_open(char *bucket_name,
                                      char *object_key,
                                      char *mode,
                                      auth_fn_t auth_func) 
{
    oss_media_file_t *file = (oss_media_file_t*)malloc(sizeof(oss_media_file_t));
    if (NULL == file) {
        aos_error_log("malloc a new file failed.\n");
        return NULL;
    }

    if (!(strcmp("r", mode) == 0 || strcmp("w", mode) == 0 
          || strcmp("a", mode) == 0)) 
    {
        free(file);
        aos_error_log("mode[%s] is wrong\n", mode);
        return NULL;
    }
    
    file->auth_func = auth_func;
    oss_auth(file, 1);
    
    file->mode = mode;
    file->_stat.pos = 0;

    file->bucket_name = bucket_name;
    file->object_key = object_key;

    if (0 != oss_media_file_stat(file, &(file->_stat))) {
        aos_error_log("stat file[%s] failed.\n", file->object_key);
        free(file);
        file = NULL;
    }
    
    return file;
}

void oss_media_file_close(oss_media_file_t *file) {
    if (NULL != file) {
        free(file);
        file = NULL;
    }
}

int oss_media_file_stat(oss_media_file_t *file, oss_media_file_stat_t *stat) {
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
    aos_str_set(&bucket, file->bucket_name);
    aos_str_set(&key, file->object_key);

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

    aos_error_log("head object[%s] failed. http code:%d, code:%s, msg:%s, req:%s",
                  file->object_key, status->code, status->error_code,
                  status->error_msg, status->req_id);
    aos_pool_destroy(pool);
    return -1;
}

int64_t oss_media_file_tell(oss_media_file_t *file) {
    if (!is_readable(file)) {
        aos_error_log("file mode[%s] is not readable\n", file->mode);
        return -1;
    }
    return file->_stat.pos;
}

int64_t oss_media_file_seek(oss_media_file_t *file, int64_t offset) {
    if (!is_readable(file)) {
        aos_error_log("file mode[%s] is not readable\n", file->mode);
        return -1;
    }
    if (offset < 0 || offset >= file->_stat.length) {
        aos_error_log("offset[%" APR_INT64_T_FMT "] is invalidate, file pos[%" APR_INT64_T_FMT "]\n", 
                      offset, file->_stat.length);
        return -1;
    }
    file->_stat.pos = offset;
    return offset;
}

int64_t oss_media_file_read(oss_media_file_t *file, void *buf, int64_t nbyte) {
    aos_pool_t *pool = NULL;
    oss_request_options_t *opts = NULL;
    aos_string_t bucket;
    aos_string_t key;
    aos_status_t *status = NULL;
    aos_table_t *req_headers = NULL;
    aos_table_t *req_params = NULL;
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
    if (file->_stat.pos > file->_stat.length - 1) {
        aos_error_log("EOF\n");
        return 0;
    }

    aos_pool_create(&pool, NULL);
    oss_init_request_opts(pool, file, &opts);
    aos_str_set(&bucket, file->bucket_name);
    aos_str_set(&key, file->object_key);
    aos_list_init(&buffer);

    end = (file->_stat.length > 0 && file->_stat.pos + nbyte > file->_stat.length) ? 
          file->_stat.length - 1 : file->_stat.pos + nbyte - 1;
    range = apr_psprintf(pool, "bytes=%" APR_INT64_T_FMT "-%" APR_INT64_T_FMT, file->_stat.pos, end);

    req_headers = aos_table_make(pool, 1);
    apr_table_set(req_headers, "Range", range);

    status = oss_get_object_to_buffer(opts, &bucket, &key, 
            req_headers, req_params, &buffer, &resp_headers);

    oss_op_debug("oss_get_object_to_buffer", status, req_headers, resp_headers);

    if (!aos_status_is_ok(status)) {
        aos_error_log("get object failed. request_id:%s, code:%d, "
                      "error_code:%s, error_message:%s",
                      status->req_id, status->code, status->error_code,
                      status->error_msg);
        aos_pool_destroy(pool);
        return -1;
    }

    aos_list_for_each_entry(aos_buf_t, content, &buffer, node) {
        size = aos_buf_size(content);
        memcpy(buf + offset, content->pos, size);
        offset += size;
    }

    len = oss_get_content_length(apr_table_get(resp_headers, "Content-Length"));
    file->_stat.pos += len;

    aos_pool_destroy(pool);
    return len;
}

int64_t oss_media_file_write(oss_media_file_t *file, const void *buf, int64_t nbyte) {
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
        aos_error_log("file mode[%s] is not [w/a]\n", file->mode);
        return -1;
    }

    aos_pool_create(&pool, NULL);
    oss_init_request_opts(pool, file, &opts);
    req_headers = aos_table_make(pool, 0);
    aos_str_set(&bucket, file->bucket_name);
    aos_str_set(&key, file->object_key);
    aos_list_init(&buffer);
    content = aos_buf_pack(pool, buf, nbyte);
    aos_list_add_tail(&content->node, &buffer);

    if (strcmp("w", file->mode) == 0) {
        status = oss_put_object_from_buffer(opts, &bucket, &key, 
                &buffer, req_headers, &resp_headers);
        
        oss_op_debug("oss_put_object_from_buffer", status, 
                  req_headers, resp_headers);

        if (!aos_status_is_ok(status)) {
            aos_error_log("put object failed. request_id:%s, code:%d, "
                          "error_code:%s, error_message:%s",
                          status->req_id, status->code, status->error_code,
                          status->error_msg);
            aos_pool_destroy(pool);
            return -1;
        }
    } else {
        status = oss_append_object_from_buffer(opts, &bucket, &key, 
                file->_stat.length, &buffer, req_headers, &resp_headers);

        oss_op_debug("oss_append_object_from_buffer", status, 
                     req_headers, resp_headers);

        if (!aos_status_is_ok(status)) {
            aos_error_log("append object failed. request_id:%s, code:%d, "
                          "error_code:%s, error_message:%s",
                          status->req_id, status->code, status->error_code,
                          status->error_msg);
            aos_pool_destroy(pool);
            return -1;
        }
        file->_stat.length = oss_get_content_length(apr_table_get(resp_headers, 
                        "x-oss-next-append-position"));
    }

    aos_pool_destroy(pool);
    return nbyte;
}

// 00 00 00 01 65 ==> Coded slice of an IDR picture
int oss_media_get_h264_idr_offsets(const void *buf, 
                                   int nbyte, 
                                   int idrs[], 
                                   int nidrs, 
                                   int *size) 
{
    if (size <= 0) {
        return 0;
    }

    int i = 0;
    int offset = 0;
    int stat = 0;
    *size = 0;

    while(offset < nbyte) {
        unsigned char *pos = (unsigned char *)(buf + (offset++));
        switch (stat) {
            case 0:
                stat = (*pos == 0x00) ? 1 : 0;
                break;
            case 1:
                stat = (*pos == 0x00) ? 2 : 0;
                break;
            case 2:
                stat = (*pos == 0x00) ? 3 : 0;
                break;
            case 3:
                stat = (*pos == 0x01) ? 4 : 0;
                break;
            case 4:
                if (*pos == 0x65) {
                    idrs[i++] = offset - stat;
                    *size = i;
                    // max idrs
                    if (*size >= nidrs) {
                        return offset - stat;
                    }
                }
                stat = 0;
                break;
            default:
                stat = 0;
        }
    }
    return offset - stat;
}
