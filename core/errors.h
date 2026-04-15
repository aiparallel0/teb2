#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    ERR_OK       = 0,
    ERR_NOT_FOUND,
    ERR_AUTH,
    ERR_INVALID,
    ERR_DB,
    ERR_IO,
    ERR_CRYPTO,
    ERR_LIMIT,
    ERR_UNKNOWN
} Err;

#endif /* ERRORS_H */
