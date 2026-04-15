#ifndef AUTH_SHA256_H
#define AUTH_SHA256_H

#include <stddef.h>

void sha256_hash(const unsigned char *data, size_t len,
                 unsigned char out[32]);

void sha256_hmac(const unsigned char *key, size_t klen,
                 const unsigned char *data, size_t dlen,
                 unsigned char out[32]);

#endif /* AUTH_SHA256_H */
