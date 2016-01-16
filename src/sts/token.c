#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "libsts.h"
#include "util.h"
#include "request.h"
#include "string_buffer.h"
#include "jsmn.h"

// sort
static void params_array_sort(char* paramsArray[], int size)
{
    int i = 0, j = 0;
    
    for (i = 0; i < size; ++i) {
        for (j = i + 1; j < size; ++j) {
            if (strcmp(paramsArray[i], paramsArray[j]) > 0) {
                char *tmp = paramsArray[i];
                paramsArray[i] = paramsArray[j];
                paramsArray[j] = tmp;
            }
        }
        
    }
}
static STSStatus compose_canonicalized_query_string(char* buffer, uint32_t bufferSize, uint32_t* len,
                                             char* paramsArray[], uint32_t paramsCount)
{
    int i = 0;
    *len = 0;
    
    char* pBuffer = &(buffer[0]);
    
    for (i = 0; i < paramsCount; ++i) {
        char* begin = paramsArray[i];
        char* middle = strchr(begin, '=');
        char* end = strchr(begin, '&');
        size_t size = 0;
        
        if (NULL != end) {
            size = end - begin + 1;
        }
        else {
            size = strlen(begin) + 1;
        }
        
        size_t keyLen = middle - begin;
        char* key = begin;
        
        size_t valueLen = size - keyLen - 2;
        char* value = middle + 1;
        
        char* encodedKey = calloc(keyLen*3, 1);
        char* encodedValue = calloc(valueLen*3, 1);
        
        percentEncode(encodedKey, key, keyLen);
        percentEncode(encodedValue, value, valueLen);
        
        snprintf(pBuffer, strlen(encodedKey)+2, "%s=", encodedKey);
        *len += strlen(encodedKey) + 1;
        pBuffer = buffer + *len;

        snprintf(pBuffer, strlen(encodedValue)+2, "%s&", encodedValue);
        *len += strlen(encodedValue) + 1;
        pBuffer = buffer + *len;
        
        free(encodedKey);
        free(encodedValue);
    }
    
    *(--pBuffer) = '\0';
    *len -= 1;
    
    return STSStatusOK;
}

static STSStatus compose_url_encoded_post_data(char* buffer, uint32_t bufferSize,
                                        char* queryParams, const uint32_t queryLen,
                                        const char* signature)
{
    char* pParams = queryParams;
    char* pBuffer = buffer;
    int len = 0;
    int index = 0;
    
    while (index < queryLen) {
        char* begin = pParams;
        char* middle = strchr(begin, '=');
        char* end = strchr(begin, '&');
        
        size_t size = 0;
        if (NULL != end) {
            size = end - begin + 1;
        }
        else {
            size = strlen(begin) + 1;
        }
        index += size;
        
        size_t keyLen = middle - begin;
        char* key = begin;
        size_t valueLen = size - keyLen - 2;
        char* value = middle + 1;
        
        char* encodedKey = calloc(keyLen*3, 1);
        char* encodedValue = calloc(valueLen*3, 1);
        
        urlEncode(encodedKey, key, keyLen);
        urlEncode(encodedValue, value, valueLen);
        
        snprintf(pBuffer, strlen(encodedKey)+2, "%s=", encodedKey);
        len += strlen(encodedKey) + 1;
        pBuffer = buffer + len;
        
        snprintf(pBuffer, strlen(encodedValue)+2, "%s&", encodedValue);
        len += strlen(encodedValue) + 1;
        pBuffer = buffer + len;
        
        pParams = end + 1;
        free(encodedKey);
        free(encodedValue);
    }
    
    char* signatureKey = "Signature=";
    size_t len2 = strlen(signatureKey);
    snprintf(pBuffer, strlen(signature)+len2+1, "Signature=%s", signature);
    
    return STSStatusOK;
}

static STSStatus compute_signature(char* buffer, uint32_t bufferSize,
                            char* paramsArray[], const uint32_t paramsCount,
                            const char* accessKeySecret)
{
    
    //////////////////////////////////////////////////////////////////////////
    // sign
    uint32_t canonLen = 0;

    char canonicalizedQueryString[2048 * 3];
    compose_canonicalized_query_string(canonicalizedQueryString, 2048 * 3, &canonLen,
                                       paramsArray, paramsCount);
    
    string_buffer(strToSign, 2048 * 3);
    string_buffer_initialize(strToSign);
    int fit;
    string_buffer_append(strToSign, "POST&%2F&", 9, fit);
    if (!fit) {
        return STSStatusUriTooLong;
    }
    
    string_buffer(percentTwice, 2048 * 3);
    string_buffer_initialize(percentTwice);
    percentEncode(percentTwice, canonicalizedQueryString, canonLen);
    string_buffer_append(strToSign, percentTwice, strlen(percentTwice), fit);

    //fprintf(stdout, "strToSign(%lu): %s\n", strlen(strToSign), strToSign); 
    
    // Generate an HMAC-SHA-1 of the strToSign
    size_t akSecretLen = strlen(accessKeySecret);
    char newAccessKeySecret[akSecretLen + 1];
    snprintf(newAccessKeySecret, akSecretLen+2, "%s&", accessKeySecret);
    
    unsigned char hmac[20];
    STS_HMAC_SHA1(hmac, (unsigned char *) newAccessKeySecret, strlen(newAccessKeySecret),
              (unsigned char *) strToSign, strlen(strToSign));
    
    // Now base-64 encode the results
    char b64[((20 + 1) * 4) / 3];
    int b64Len = base64Encode(hmac, 20, b64);

    char b64Encoded[256];
    if (!urlEncode(b64Encoded, b64, b64Len)) {
        return STSStatusUriTooLong;
    }
    
    snprintf(buffer, strlen(b64Encoded)+1, "%s", b64Encoded);
    
    return STSStatusOK;
    
}

static STSStatus compose_post_data(char* buffer, uint32_t bufferSize,
                            CommonRequestContext* commonContext,
                            AssumeRoleRequestContext* assumeRoleContext)
{
    string_buffer(queryParams, 2048*3);
    string_buffer_initialize(queryParams);
    char* queryParamsArray[32];
    int paramsCount = 0;
    int len = 0;
    int start = 0;
    int amp = 0;

#define safe_append(isNewParam, name, value)                            \
    do {                                                                \
        int fit;                                                        \
        start = len;                                                    \
        if (amp) { start++; }                                           \
        if (isNewParam) {                                               \
            queryParamsArray[paramsCount++] = &(queryParams[start]);    \
        }                                                               \
        if (amp) {                                                      \
            string_buffer_append(queryParams, "&", 1, fit);             \
            if (!fit) {                                                 \
                return STSStatusUriTooLong;                             \
            }                                                           \
            len++;                                                      \
        }                                                               \
        string_buffer_append(queryParams, name "=",                     \
                            sizeof(name "=") - 1, fit);                 \
        if (!fit) {                                                     \
            return STSStatusUriTooLong;                                 \
        }                                                               \
        len += strlen(name) + 1;                                        \
        amp = 1;                                                        \
        string_buffer_append(queryParams, value, strlen(value), fit);   \
        len += strlen(value);                                           \
    } while (0)

    time_t now = time(NULL);
    char timebuf[256];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    
    STSUUID uuid;
    char* uuidString;
    uuid_create(&uuid);
    uuid_to_string(&uuid, &uuidString);
    
    char durationSeconds[10];
    snprintf(durationSeconds, 10, "%d", assumeRoleContext->durationSeconds);

    safe_append(1, "Format", "JSON");
    safe_append(1, "Version", "2015-04-01");
    safe_append(1, "SignatureVersion", "1.0");
    safe_append(1, "SignatureMethod", "HMAC-SHA1");
    safe_append(1, "SignatureNonce", uuidString);
    safe_append(1, "Timestamp", timebuf);
    safe_append(1, "Action", commonContext->action);
    safe_append(1, "AccessKeyId", commonContext->accessKeyId);
    safe_append(1, "RoleArn", assumeRoleContext->RoleArn);
    safe_append(1, "RoleSessionName", assumeRoleContext->RoleSessionName);
    safe_append(1, "Policy", assumeRoleContext->policy);
    safe_append(1, "DurationSeconds", durationSeconds);
    
    params_array_sort(queryParamsArray, paramsCount);
    
    string_buffer(signature, 256);
    string_buffer_initialize(signature);
    compute_signature(signature, 256, queryParamsArray, paramsCount, commonContext->accessKeySecret);
    
    compose_url_encoded_post_data(buffer, bufferSize, queryParams, strlen(queryParams), signature);
    
    free(uuidString);
    return STSStatusOK;
}

static STSStatus parse_sts_result(int bufferSize, const char* buffer, STSData* result)
{
    STSData* stsData = (STSData*)result;
    jsmntok_t json[256];
    jsmn_parser parser;
    jsmnerr_t r;
    int i = 0;
    int keyLen = 0;
    int valueLen = 0;
    const char* pKey;
    const char* pValue;
    
    jsmn_init(&parser);
    r = jsmn_parse(&parser, buffer, bufferSize, json, 256);
    
    if (r >= 0) {
        for (i = 0; i < r; ++i) {
            if (JSMN_STRING == json[i].type) {
                keyLen = json[i].end - json[i].start;
                valueLen = json[i+1].end - json[i+1].start;
                pKey = &buffer[json[i].start];
                pValue = &buffer[json[i+1].start];
                
                if (0 == strncmp("AccessKeyId", pKey, keyLen)) {
                    snprintf(stsData->tmpAccessKeyId, valueLen+1, "%s", pValue);
                }
                else if (0 == strncmp("AccessKeySecret", pKey, keyLen)) {
                    snprintf(stsData->tmpAccessKeySecret, valueLen+1, "%s", pValue);
                }
                else if (0 == strncmp("SecurityToken", pKey, keyLen)) {
                    snprintf(stsData->securityToken, valueLen+1, "%s", pValue);
                }
            }
        }
    }
    
    return STSStatusOK;
}

STSStatus STS_assume_role(const char* roleArn, const char* roleSessionName,
                          const char* policy, const uint32_t durationSeconds,
                          const char* accessKeyId, const char* accessKeySecret,
                          STSData* assumeRoleData, char* errorMessage)
{
    int status;
    int httpResponseCode;
    CommonRequestContext commonContext = {STSProtocolHTTPS, STS_DEFAULT_HOSTNAME,
                                          accessKeyId, accessKeySecret, "AssumeRole"};
    
    AssumeRoleRequestContext assumeRoleContext = {roleArn, roleSessionName, policy, durationSeconds};
    
    string_buffer(response, MAX_RESPONSE_SIZE);
    string_buffer_initialize(response);
    
    string_buffer(postData, 2048*3);
    string_buffer_initialize(postData);
    
    compose_post_data(postData, 2048*3, &commonContext, &assumeRoleContext);
    
    RequestParams params =
    {
        HttpRequestTypePOST,                    // httpRequestType
        commonContext,                          // commonContext
        assumeRoleContext,                      // assumeRoleContext
        postData                                // postData
    };
    
    status = request_perform(&params, &httpResponseCode, response);
    
    if (200 == httpResponseCode) {
        parse_sts_result(strlen(response), response, assumeRoleData);
    }
    else if( 0 != httpResponseCode)
    {
        if (400 == httpResponseCode) {
            status = STSStatusInvalidParameter;
        }
        else if (403 == httpResponseCode) {
            status = STSStatusNoPermission;
        }
        else if (500 == httpResponseCode) {
            status = STSStatusInternalError;
        }
        else if (404 == httpResponseCode) {
            status = STSStatusInvalidAcccessKeyId;
        }
        snprintf(errorMessage, strlen(response)+1, "%s", response);
    }
    else if (0 == httpResponseCode) {
        switch (status) {
            case STSStatusOutOfMemory:
                snprintf(errorMessage, 256, "%s.", "Out of memory");
                break;
                
            case STSStatusNameLookupError:
                snprintf(errorMessage, 256, "%s %s.", 
                        "Failed to lookup STS server:", 
                        commonContext.hostname);
                break;
                
            case STSStatusFailedToConnect:
                snprintf(errorMessage, 256, "%s %s.", 
                        "Failed to connect to STS server:", 
                        commonContext.hostname);
                break;
                
            case STSStatusConnectionFailed:
                snprintf(errorMessage, 256, "%s.", 
                        "Write Error or Operation Timeout");
                break;
                
            case STSStatusServerFailedVerification:
                snprintf(errorMessage, 256, "%s.", 
                        "SSL verification failed");
                break;
                
            case STSStatusInternalError:
                snprintf(errorMessage, 256, "%s.", "Internal Error");
                break;
                
            default:
                break;
        }
    }
    return status;
}
