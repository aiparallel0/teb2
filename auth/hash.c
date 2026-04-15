#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <crypt.h>
#include <fcntl.h>
#include <unistd.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"

#define SALT_BYTES 12

/* Read random bytes from /dev/urandom for salt generation */
static int read_random(unsigned char *buf, size_t len)
{
    int fd;
    ssize_t n;
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    n = read(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

/* Build salt string for SHA-512 crypt with random salt */
static void make_salt(const HashConfig *cfg, char out[64])
{
    snprintf(out, 64, "$6$rounds=%u$%s$",
             cfg->rounds ? cfg->rounds : 5000,
             cfg->salt[0] ? cfg->salt : "teb2default");
}

HashConfig default_hash_config(void)
{
    HashConfig cfg;
    unsigned char raw[SALT_BYTES];
    size_t i;
    memset(&cfg, 0, sizeof(cfg));
    cfg.rounds = 5000;
    if (read_random(raw, sizeof(raw)) == 0) {
        for (i = 0; i < SALT_BYTES && i * 2 + 2 < sizeof(cfg.salt);
             i++)
            snprintf(cfg.salt + i * 2, 3, "%02x", raw[i]);
    } else {
        snprintf(cfg.salt, sizeof(cfg.salt), "%s", "fallback");
    }
    return cfg;
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
