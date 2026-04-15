#ifndef API_JSON_H
#define API_JSON_H

#include "core/types.h"

HttpResp json_error(int status, const char *msg);
HttpResp json_ok(const char *body);
int extract_json_str(const char *body, const char *key,
                     char *out, size_t outsz);

#endif /* API_JSON_H */
