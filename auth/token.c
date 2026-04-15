#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <time.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"

#define TOKEN_TTL 3600L

#ifdef TEB2_MODERN
#include "auth/sha256.h"
/*
 * HMAC-SHA256 MAC over ticket fields + secret.
 * Produces a full 32-byte MAC stored in ticket.mac[32].
 */
static void compute_mac(int64_t user_id, UserRole role, int64_t expiry,
                        const char *secret, unsigned char mac[32])
{
    unsigned char data[32];
    memset(data, 0, sizeof(data));
    memcpy(data,      &user_id, sizeof(user_id));
    memcpy(data + 8,  &role,    sizeof(role));
    memcpy(data + 12, &expiry,  sizeof(expiry));
    sha256_hmac((const unsigned char *)secret, strlen(secret),
                data, sizeof(data), mac);
}
#else
/*
 * Legacy XOR-fold MAC.  NOT cryptographic — use -DTEB2_MODERN for
 * HMAC-SHA256.  Kept for backward compatibility on constrained builds.
 */
static void compute_mac(int64_t user_id, UserRole role, int64_t expiry,
                        const char *secret, unsigned char mac[32])
{
    unsigned char buf[32];
    size_t i, slen = strlen(secret);
    memset(mac, 0, 32);
    memset(buf, 0, sizeof(buf));
    memcpy(buf,      &user_id, sizeof(user_id));
    memcpy(buf + 8,  &role,    sizeof(role));
    memcpy(buf + 12, &expiry,  sizeof(expiry));
    for (i = 0; i < slen && i < sizeof(buf); i++)
        buf[i % sizeof(buf)] ^= (unsigned char)secret[i];
    for (i = 0; i < 8; i++)
        mac[i] = buf[i] ^ buf[i + 8] ^ buf[i + 16] ^ buf[i + 24];
}
#endif

TokenResult make_ticket(UserClaims claims, const char *secret)
{
    TokenResult r;
    memset(&r, 0, sizeof(r));
    if (!secret || secret[0] == '\0') {
        r.err = ERR_CRYPTO;
        return r;
    }
    r.ticket.user_id = claims.user_id;
    r.ticket.role    = claims.role;
    r.ticket.expiry  = (int64_t)time(NULL) + TOKEN_TTL;
    compute_mac(r.ticket.user_id, r.ticket.role, r.ticket.expiry,
                secret, r.ticket.mac);
    r.claims = claims;
    r.err    = ERR_OK;
    return r;
}

TokenResult check_ticket(Ticket t, const char *secret)
{
    TokenResult r;
    unsigned char expected[32];
    int64_t now;
    size_t i;
    int diff;
    memset(&r, 0, sizeof(r));
    if (!secret || secret[0] == '\0') {
        r.err = ERR_CRYPTO;
        return r;
    }
    now = (int64_t)time(NULL);
    if (t.expiry < now) {
        r.err = ERR_AUTH;
        return r;
    }
    compute_mac(t.user_id, t.role, t.expiry, secret, expected);
    diff = 0;
    for (i = 0; i < 32; i++)
        diff |= (int)(t.mac[i] ^ expected[i]);
    if (diff != 0) {
        r.err = ERR_AUTH;
        return r;
    }
    r.ticket         = t;
    r.claims.user_id = t.user_id;
    r.claims.role    = t.role;
    r.err            = ERR_OK;
    return r;
}
