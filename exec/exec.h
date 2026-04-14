#ifndef EXEC_H
#define EXEC_H

#include "core/types.h"
#include "core/errors.h"

CipherResult  vault_encrypt(Bytes plaintext, Key8 key)
    __attribute__((warn_unused_result));

CipherResult  vault_decrypt(Bytes ciphertext, Key8 key)
    __attribute__((warn_unused_result));

HttpResult    send_request(HttpReq req, Cred *cred)
    __attribute__((warn_unused_result));

BrowserResult browser_send(SerialCmd cmd, Cred *cred)
    __attribute__((warn_unused_result));

#endif /* EXEC_H */
