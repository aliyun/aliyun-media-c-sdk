#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "request.h"
#include "util.h"


static STSStatus compose_uri(char* buffer, int bufferSize, const CommonRequestContext* commonContext)
{
    int len = 0;
    
#define uri_append(fmt, ...)                                                 \
    do {                                                                     \
            len += snprintf(&(buffer[len]), bufferSize - len, fmt, __VA_ARGS__); \
            if (len >= bufferSize) {                                             \
                return STSStatusUriTooLong;                                       \
            }                                                                    \
    } while (0)
    
    uri_append("http%s://", (STSProtocolHTTP == commonContext->protocol) ? "" : "s");
    
    const char* hostName = commonContext->hostname ? commonContext->hostname : STS_DEFAULT_HOSTNAME;
    
    uri_append("%s", hostName);
    
    return STSStatusOK;
}

static void request_headers_done(Request *request)
{
    // Get the http response code
    long httpResponseCode;
    request->httpResponseCode = 0;
    if (curl_easy_getinfo(request->curl, CURLINFO_RESPONSE_CODE,
                          &httpResponseCode) != CURLE_OK) {
        // Not able to get the HTTP response code - error
        request->status = STSStatusInternalError;
        return;
    }
    else {
        request->httpResponseCode = httpResponseCode;
    }
}

static size_t curl_write_func(void *ptr, size_t size, size_t nmemb,
                              void *data)
{
    Request *request = (Request *) data;
    
    int len = size * nmemb;
    
    request_headers_done(request);
    
    //if (request->status != STSStatusOK) {
    //    return 0;
    //}
    
    // On HTTP error, we expect to parse an HTTP error response
    //if ((request->httpResponseCode < 200) ||
    //    (request->httpResponseCode > 299)) {
        //request->status = error_parser_add(&(request->errorParser), (char *) ptr, len);
    //}
    // If there was a callback registered, make it
    //else
    
    if (request->getResultCallback) {
        request->status = (*(request->getResultCallback))(len, (char *) ptr, request->callbackData);
    }
    // Else, consider this an error - S3 has sent back data when it was not
    // expected
    else {
        request->status = STSStatusInternalError;
    }
    
    return ((request->status == STSStatusOK) ? len : 0);
}



static STSStatus setup_curl(Request* request, RequestParams* params)
{
    STSStatus status;
 
#define curl_easy_setopt_safe(opt, val)                                 \
    if ((status = curl_easy_setopt                                      \
        (request->curl, opt, val)) != CURLE_OK) {                      \
            return STSStatusFailedToInitRequest;                       \
    }
    
    curl_easy_setopt_safe(CURLOPT_PRIVATE, request);
    curl_easy_setopt_safe(CURLOPT_URL, request->uri);
    //curl_easy_setopt_safe(CURLOPT_SSL_VERIFYPEER, 0);
    //curl_easy_setopt_safe(CURLOPT_SSL_VERIFYHOST, 0);
    //curl_easy_setopt_safe(CURLOPT_VERBOSE, 1);
    curl_easy_setopt_safe(CURLOPT_TIMEOUT, STS_HTTP_TIMEOUT);
    curl_easy_setopt_safe(CURLOPT_POST, 1);
    curl_easy_setopt_safe(CURLOPT_POSTFIELDS, params->postData);
    curl_easy_setopt_safe(CURLOPT_POSTFIELDSIZE, strlen(params->postData));
    
    // Set write callback and data
    curl_easy_setopt_safe(CURLOPT_WRITEFUNCTION, &curl_write_func);
    curl_easy_setopt_safe(CURLOPT_WRITEDATA, request);
    
    request->headers = curl_slist_append(request->headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(request->curl, CURLOPT_HTTPHEADER, request->headers);
    
    return status;
}

STSStatus request_get(RequestParams* params, Request** requestReturn)
{
    Request* request;
    STSStatus status;
    
    if (!(request = (Request*)malloc(sizeof(Request)))) {
        return STSStatusOutOfMemory;
    }
    
    request->headers = 0;
    
    if (!(request->curl = curl_easy_init())) {
        return STSStatusFailedToInitRequest;
    }
    
    if (STSStatusOK != (status = compose_uri(request->uri, sizeof(request->uri), &(params->commonContext)))) {
        curl_easy_cleanup(request->curl);
        free(request);
        return status;
    }
    
    if (STSStatusOK != (status = setup_curl(request, params))) {
        curl_easy_cleanup(request->curl);
        free(request);
        return status;
    }
               
    *requestReturn = request;
    
    return status;
}

static void request_deinitialize(Request *request)
{
    if (request->headers) {
        curl_slist_free_all(request->headers);
    }
    
    //error_parser_deinitialize(&(request->errorParser));
    
}

static void request_destroy(Request *request)
{
    request_deinitialize(request);
    curl_easy_cleanup(request->curl);
    free(request);
}

void request_finish(Request *request)
{
    request_destroy(request);
}

static STSStatus responseCallback(int bufferSize, const char* buffer,
                                  void* callbackData)
{
    char* response = (char*)callbackData;
    
    if (bufferSize > MAX_RESPONSE_SIZE) {
        return STSStatusResponseToLarge;
    }
    else {
        snprintf(response, bufferSize+1, "%s", buffer);
    }
        
    return STSStatusOK;
}

STSStatus request_curl_code_to_status(CURLcode code)
{
    switch (code) {
        case CURLE_OUT_OF_MEMORY:
            return STSStatusOutOfMemory;
            
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_RESOLVE_HOST:
            return STSStatusNameLookupError;
        
        case CURLE_COULDNT_CONNECT:
            return STSStatusFailedToConnect;
        
        case CURLE_WRITE_ERROR:
        case CURLE_OPERATION_TIMEDOUT:
            return STSStatusConnectionFailed;
        
        case CURLE_PARTIAL_FILE:
            return STSStatusOK;
        
        case CURLE_SSL_CACERT:
            return STSStatusServerFailedVerification;
        
        default:
            return STSStatusInternalError;
    }
}

STSStatus request_perform(RequestParams* params, int* httpResponseCode, char* response)
{
    Request* request = 0;
    STSStatus status;
    
    
    if (STSStatusOK != (status = request_get(params, &request))) {
        fprintf(stderr, "Request Get ERR\n");
        return status;
    }
    
    request->getResultCallback = responseCallback;
    request->callbackData = response;
    request->status = STSStatusWaiting;
    
    CURLcode code = curl_easy_perform(request->curl);
    
    if (CURLE_OK != code) {
        request->httpResponseCode = 0;
        *httpResponseCode = request->httpResponseCode;
        request->status = request_curl_code_to_status(code);
        //fprintf(stderr, "Request Perform ERR\n");
        return request->status;
    }
    else {
        *httpResponseCode = request->httpResponseCode;
        //fprintf(stdout, "\nRequest Perform OK\n");
        request_finish(request);
        return request->status;
    }
}
