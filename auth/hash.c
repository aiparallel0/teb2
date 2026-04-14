#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <crypt.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"

/* Build salt string for SHA-512 crypt */
static void make_salt(const HashConfig *cfg, char out[64])
{
    snprintf(out, 64, "$6$rounds=%u$%s$", cfg->rounds ? cfg->rounds : 5000,
             cfg->salt[0] ? cfg->salt : "teb2default");
}

HashResult hash_password(const char *password, HashConfig cfg)
{
    HashResult r;
    char salt[64];
    const char *h;

    memset(&r, 0, sizeof(r));
    if (!password || password[0] == '\0') {
        r.err = ERR_CRYPTO;
        return r;
    }
    make_salt(&cfg, salt);
    h = crypt(password, salt);
    if (!h) {
        r.err = ERR_CRYPTO;
        return r;
    }
    snprintf(r.hash, sizeof(r.hash), "%s", h);
    r.err = ERR_OK;
    return r;
}

HashResult verify_password(const char *password, HashResult stored)
{
    HashResult r;
    const char *h;

    memset(&r, 0, sizeof(r));
    if (!password || stored.hash[0] == '\0') {
        r.err = ERR_AUTH;
        return r;
    }
    /* re-hash with the stored hash as the salt (crypt extracts it) */
    h = crypt(password, stored.hash);
    if (!h) {
        r.err = ERR_CRYPTO;
        return r;
    }
    r.match = (strcmp(h, stored.hash) == 0) ? 1 : 0;
    r.err   = ERR_OK;
    snprintf(r.hash, sizeof(r.hash), "%s", stored.hash);
    return r;
}
