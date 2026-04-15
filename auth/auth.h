#ifndef AUTH_H
#define AUTH_H

#include "core/types.h"
#include "core/errors.h"

HashConfig  default_hash_config(void);

HashResult  hash_password(const char *password, HashConfig cfg)
    __attribute__((warn_unused_result));

HashResult  verify_password(const char *password, HashResult stored)
    __attribute__((warn_unused_result));

TokenResult make_ticket(UserClaims claims, const char *secret)
    __attribute__((warn_unused_result));

TokenResult check_ticket(Ticket t, const char *secret)
    __attribute__((warn_unused_result));

int rbac_allow(UserRole role, Permission required);

#endif /* AUTH_H */
