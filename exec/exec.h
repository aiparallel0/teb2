#ifndef EXEC_H
#define EXEC_H

#include "core/types.h"
#include "core/errors.h"
#include "core/types_collab.h"

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

#endif /* EXEC_H */
