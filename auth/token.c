#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <time.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"

#define TOKEN_TTL 3600L

/* Simple 8-byte MAC: XOR-fold of a 32-byte mixing pass over the fields */
static void compute_mac(int64_t user_id, UserRole role, int64_t expiry,
                        const char *secret, unsigned char mac[8])
{
    unsigned char buf[32];
    size_t i;
    size_t slen = strlen(secret);

    memset(buf, 0, sizeof(buf));
    memcpy(buf,      &user_id, sizeof(user_id));
    memcpy(buf + 8,  &role,    sizeof(role));
    memcpy(buf + 12, &expiry,  sizeof(expiry));

    for (i = 0; i < slen && i < sizeof(buf); i++)
        buf[i % sizeof(buf)] ^= (unsigned char)secret[i];

    for (i = 0; i < 8; i++)
        mac[i] = buf[i] ^ buf[i + 8] ^ buf[i + 16] ^ buf[i + 24];
}

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
    unsigned char expected[8];
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
    for (i = 0; i < 8; i++)
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
