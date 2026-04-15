#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"
#include "exec/exec.h"

static void hex32(const unsigned char *src, char *dst)
{
    int i;
    for (i = 0; i < 16; i++)
        snprintf(dst + i * 2, 3, "%02x", src[i]);
    dst[32] = '\0';
}

static AgentMsg handle_state(AgentMsg msg)
{
    unsigned char rnd[16];
    char hex[33];
    char prov[32];
    MemQuery mq;
    MemResult mr;
    int fd;
    ssize_t n;

    snprintf(prov, sizeof(prov), "%.*s", 31, msg.payload + 6);
    memset(rnd, 0, sizeof(rnd));
    fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        n = read(fd, rnd, sizeof(rnd));
        (void)n;
        close(fd);
    }
    hex32(rnd, hex);
    if (msg.db) {
        memset(&mq, 0, sizeof(mq));
        snprintf(mq.agent, sizeof(mq.agent), "oauth");
        snprintf(mq.key, sizeof(mq.key), "state_%s", prov);
        snprintf(mq.val, sizeof(mq.val), "%s", hex);
        mr = store_mem(msg.db, mq);
        (void)mr;
    }
    return agent_make_result(msg, ERR_OK, hex);
}

static AgentMsg handle_exchange(AgentMsg msg)
{
    char prov[32], code[256], ruri[256];
    const char *p = msg.payload + 9;
    const char *c1, *c2;
    OAuthResult r;

    memset(prov, 0, sizeof(prov)); memset(code, 0, sizeof(code));
    memset(ruri, 0, sizeof(ruri));
    c1 = strchr(p, ':');
    if (!c1) return agent_make_result(msg, ERR_UNKNOWN, "bad_exchange");
    snprintf(prov, sizeof(prov), "%.*s", (int)(c1 - p), p);
    c2 = strchr(c1 + 1, ':');
    if (!c2) return agent_make_result(msg, ERR_UNKNOWN, "bad_exchange");
    snprintf(code, sizeof(code), "%.*s", (int)(c2 - c1 - 1), c1 + 1);
    snprintf(ruri, sizeof(ruri), "%s", c2 + 1);
    r = oauth_exchange(prov, code, ruri, NULL, msg.db,  msg.user_id);
    if (r.err != ERR_OK)
        return agent_make_result(msg, r.err, "exchange_failed");
    return agent_make_result(msg, ERR_OK, r.tok.access_tok);
}

static AgentMsg handle_refresh(AgentMsg msg)
{
    char prov[32], rtok[256];
    const char *p = msg.payload + 8;
    const char *c1;
    OAuthResult r;

    memset(prov, 0, sizeof(prov)); memset(rtok, 0, sizeof(rtok));
    c1 = strchr(p, ':');
    if (!c1) return agent_make_result(msg, ERR_UNKNOWN, "bad_refresh");
    snprintf(prov, sizeof(prov), "%.*s", (int)(c1 - p), p);
    snprintf(rtok, sizeof(rtok), "%s", c1 + 1);
    r = oauth_refresh(prov, rtok, NULL, msg.db, msg.user_id);
    if (r.err != ERR_OK)
        return agent_make_result(msg, r.err, "refresh_failed");
    return agent_make_result(msg, ERR_OK, r.tok.access_tok);
}

AgentMsg oauth_handle(AgentMsg msg)
{
    if (msg.tag != MSG_OAUTH)
        return agent_make_result(msg, ERR_UNKNOWN, "not_an_oauth_request");
    if (strncmp(msg.payload, "state:", 6) == 0)
        return handle_state(msg);
    if (strncmp(msg.payload, "exchange:", 9) == 0)
        return handle_exchange(msg);
    if (strncmp(msg.payload, "refresh:", 8) == 0)
        return handle_refresh(msg);
    return agent_make_result(msg, ERR_OK, "state_generated");
}
