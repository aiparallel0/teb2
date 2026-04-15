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

#endif /* EXEC_H */
