#ifndef EXEC_H
#define EXEC_H

#include <sys/types.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/types_collab.h"
#include "core/types_ext.h"

CipherResult  vault_encrypt(Bytes plaintext, Key8 key)
    __attribute__((warn_unused_result));

CipherResult  vault_decrypt(Bytes ciphertext, Key8 key)
    __attribute__((warn_unused_result));

HttpResult    send_request(HttpReq req, Cred *cred)
    __attribute__((warn_unused_result));

BrowserResult browser_send(SerialCmd cmd, Cred *cred)
    __attribute__((warn_unused_result));

NotifyResult  send_notify(NotifyReq req, Cred *cred)
    __attribute__((warn_unused_result));

typedef struct { int fd; int open; } SseConn;
void        sse_write(SseConn *c, const char *event, const char *data);
void        sse_close(SseConn *c);

/* smtp */
typedef struct { char to[256]; char subject[256]; char body[1024]; } MailReq;
typedef struct { Err err; int sent; } MailResult;
MailResult  send_mail(MailReq req, Config *cfg)
    __attribute__((warn_unused_result));

/* oauth */
OAuthResult oauth_exchange(const char *provider, const char *code,
    const char *redirect_uri, Config *cfg, Db *db, const char *user_id)
    __attribute__((warn_unused_result));
OAuthResult oauth_refresh(const char *provider, const char *refresh_tok,
    Config *cfg, Db *db, const char *user_id)
    __attribute__((warn_unused_result));

/* browser process lifecycle */
typedef struct { int pid; int rfd; int wfd; } BrowserProc;
int browser_spawn(BrowserProc *proc) __attribute__((warn_unused_result));

/* exec/tls.c — verified TLS client. Returns bytes read (>=0) or -1. */
ssize_t tls_request(const char *host, int port,
                    const char *req_buf, size_t req_len,
                    char *resp_buf, size_t resp_cap)
    __attribute__((warn_unused_result));

/* exec/run_spawn.c — fork a child to execute a workflow run.
 * Owns all I/O: fork(), signal(SIGALRM), alarm(), child db_open,
 * execute_steps, update_run_status, db_close; parent update_run_pid
 * and update_run_status("running"). */
typedef struct {
    int64_t    run_id;
    int        timeout_sec;
    TaskResult tasks;
} RunSpawnReq;

typedef struct { Err err; int pid; } RunSpawnResult;

RunSpawnResult exec_run_spawn(RunSpawnReq req, Ctx *ctx)
    __attribute__((warn_unused_result));

#endif /* EXEC_H */
