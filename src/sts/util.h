#ifndef _STS_UTIL_H_
#define _STS_UTIL_H_

#include <curl/curl.h>
#include <curl/multi.h>
#include <stdint.h>
#include <pthread.h>

#include "libsts.h"

/* Length of a node address (an IEEE 802 address). */
#define UUID_NODE_LEN 6

typedef struct STSUUID {
    uint32_t        time_low;
    uint16_t        time_mid;
    uint16_t        time_hi_and_version;
    uint8_t         clock_seq_hi_and_reserved;
    uint8_t         clock_seq_low;
    uint8_t         node[UUID_NODE_LEN];
} STSUUID;

void uuid_create(STSUUID* uuid);

void uuid_to_string(STSUUID* uuid, char** str);

uint64_t getRealTime();

// Utilities -----------------------------------------------------------------

int percentEncode(char* dest, const char *src, int maxSrcSize);

// URL-encodes a string from [src] into [dest].  [dest] must have at least
// 3x the number of characters that [source] has.   At most [maxSrcSize] bytes
// from [src] are encoded; if more are present in [src], 0 is returned from
// urlEncode, else nonzero is returned.
int urlEncode(char *dest, const char *src, int maxSrcSize);

// Returns < 0 on failure >= 0 on success
int64_t parseIso8601Time(const char *str);

uint64_t parseUnsignedInt(const char *str);

// base64 encode bytes.  The output buffer must have at least
// ((4 * (inLen + 1)) / 3) bytes in it.  Returns the number of bytes written
// to [out].
int base64Encode(const unsigned char *in, int inLen, char *out);

// Compute HMAC-SHA-1 with key [key] and message [message], storing result
// in [hmac]
void HMAC_SHA1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len);

void STS_HMAC_SHA1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len);

// Compute a 64-bit hash values given a set of bytes
uint64_t hash(const unsigned char *k, int length);

// Because Windows seems to be missing isblank(), use our own; it's a very
// easy function to write in any case
int is_blank(char c);

#endif
