#ifndef _STS_REQEUST_H_
#define _STS_REQEUST_H_

#include "libsts.h"
#include "util.h"

typedef enum
{
    HttpRequestTypeGET,
    HttpRequestTypePOST,
    HttpRequestTypeHEAD,
    HttpRequestTypePUT,
    HttpRequestTypeCOPY,
    HttpRequestTypeDELETE
} HttpRequestType;

typedef struct RequestParams
{
    HttpRequestType httpRequestType;
    
    CommonRequestContext commonContext;
    
    AssumeRoleRequestContext assumeRoleContext;
    
    const char* postData;
    
} RequestParams;

typedef struct Request
{
    CURL* curl;
    
    struct curl_slist *headers;
    
    char uri[MAX_URI_SIZE + 1];
    
    int httpResponseCode;
    
    STSStatus status;
    
    STSGetResultCallback* getResultCallback;
    
    void* callbackData;
    
} Request;

STSStatus request_perform(RequestParams* params, int* httpResponseCode, char* response);



#endif
