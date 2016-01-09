#include "oss_c_sdk/aos_status.h"
#include "oss_c_sdk/aos_log.h"

void oss_op_debug(char *op, 
                  aos_status_t *status, 
                  aos_table_t *req_headers, 
                  aos_table_t *resp_headers) 
{
    if(aos_log_level >= AOS_LOG_DEBUG) {
        int pos;
        aos_array_header_t *tarr;
        aos_table_entry_t  *telts;

        if (NULL != op && NULL != status) {
            aos_debug_log("req_id:%s \n", status->req_id);
            aos_debug_log("%s status:\n", op);
            aos_debug_log("    [code=%d, err_code=%s, err_msg=%s]\n", 
                          status->code, status->error_code, status->error_msg);
        }

        if (NULL != req_headers) {
            aos_debug_log("request headers:\n");
            tarr = (aos_array_header_t *)aos_table_elts(req_headers);
            telts = (aos_table_entry_t*) tarr->elts;
            for (pos = 0; pos < tarr->nelts; pos++) {
                aos_debug_log("    %s=%s\n", telts[pos].key, telts[pos].val);
            }
        }
        
        if (NULL != resp_headers) {
            aos_debug_log("response headers:\n");
            tarr = (aos_array_header_t *)aos_table_elts(resp_headers);
            telts = (aos_table_entry_t*) tarr->elts;
            for (pos = 0; pos < tarr->nelts; pos++) {
                aos_debug_log("    %s=%s\n", telts[pos].key, telts[pos].val);
            }
        }
    }
}

void oss_op_error(char *op, aos_status_t *status)
{
    aos_error_log("req_id:%s \n", status->req_id);
    aos_error_log("%s status:\n", op);
    aos_error_log("\t[code=%d, err_code=%s, err_msg=%s]\n", 
                  status->code, status->error_code, status->error_msg);
}
