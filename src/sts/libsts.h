#ifndef _LIB_STS_H_
#define _LIB_STS_H_

#include <stdint.h>

#ifdef __cplusplus__
extern "C" {
#endif

/**
 *  * This is the default hostname that is being used for the STS requests
 *   
 **/
#define STS_DEFAULT_HOSTNAME "sts.aliyuncs.com"
    
#define MAX_URI_SIZE 1024
    
#define MAX_RESPONSE_SIZE 1024*10
    
#define STS_HTTP_TIMEOUT 5
    
    

typedef enum
{
    STSStatusOK,
    
    STSStatusOutOfMemory,
    
    STSStatusFailedToInitRequest,
    
    STSStatusUriTooLong,
    
    STSStatusResponseToLarge,
    
    STSStatusWaiting,
    
    STSStatusInternalError,
    
    STSStatusHttpErrorBadRequest,
    
    STSStatusNameLookupError,
    
    STSStatusFailedToConnect,
    
    STSStatusConnectionFailed,
    
    STSStatusServerFailedVerification,
    
    STSStatusHttpErrorInvalidParameter,
    
    STSStatusHttpErrorNoPermission,
    
    STSStatusHttpErrorInternalError,
    
    STSStatusHttpErrorUnknown,
    
    STSStatusInvalidParameter,
    
    STSStatusNoPermission,
    
    STSStatusInvalidAcccessKeyId
    
} STSStatus;

typedef enum
{
    STSProtocolHTTPS = 0,
    
    STSProtocolHTTP = 1
} STSProtocol;
    
typedef struct CommonRequestContext
{
    STSProtocol protocol;
    
    const char* hostname;
    
    const char* accessKeyId;
    
    const char* accessKeySecret;
    
    const char* action;
    
} CommonRequestContext;
    
typedef struct AssumeRoleRequestContext
{
    const char* RoleArn;
    
    const char* RoleSessionName;
    
    const char* policy;
    
    uint32_t durationSeconds;

} AssumeRoleRequestContext;
    
typedef struct STSData
{
    char tmpAccessKeyId[64];
    char tmpAccessKeySecret[64];
    char securityToken[2048];
} STSData;
    
typedef STSStatus (STSGetResultCallback)(int bufferSize, const char* buffer,
                                         void* callbackData);

STSStatus STS_assume_role(const char* roleArn, const char* roleSessionName,
                          const char* policy, const uint32_t durationSeconds,
                          const char* accessKeyId, const char* accessKeySecret,
                          STSData* stsData, char* errorMessage);



#ifdef __cplusplus
}
#endif

#endif
