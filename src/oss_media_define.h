#ifndef OSS_MEDIA_DEFINE_H
#define OSS_MEDIA_DEFINE_H

#ifdef __cplusplus
# define OSS_MEDIA_CPP_START extern "C" {
# define OSS_MEDIA_CPP_END }
#else
# define OSS_MEDIA_CPP_START
# define OSS_MEDIA_CPP_END
#endif

extern char OSS_MEDIA_USER_AGENT[];

extern int OSS_MEDIA_FILE_NOT_FOUND;
extern char OSS_MEDIA_FILE_UNKNOWN_TYPE[];

extern int OSS_MEDIA_FILE_NUM_PER_LIST;
extern int OSS_MEDIA_INTERNAL_ERROR_CODE;
extern int OSS_MEDIA_OK;

#endif
