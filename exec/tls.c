#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

/*
 * Thin TLS client. One call: tls_request() dials host:port, performs a
 * verified TLS handshake against the system trust store, writes req_buf,
 * drains the response into resp_buf, and returns the byte count. Returns
 * -1 on any failure (DNS, connect, handshake, cert verify, I/O).
 *
 * Certificate verification is ON by default (SSL_VERIFY_PEER + hostname
 * check). SNI is sent. TLS 1.2 minimum. No custom cipher list -- we
 * trust the OpenSSL default policy which tracks current CA/B Forum.
 *
 * Kept deliberately small so exec/http.c can upgrade https:// calls
 * without growing past 166 LOC. A single-shot context is created per
 * request; pooling would require a real connection manager which does
 * not exist yet (and http.c still uses HTTP/1.0 close-on-done semantics).
 */

static SSL_CTX *make_ctx(void)
{
    const SSL_METHOD *m = TLS_client_method();
    SSL_CTX *c = SSL_CTX_new(m);
    if (!c) return NULL;
    SSL_CTX_set_min_proto_version(c, TLS1_2_VERSION);
    SSL_CTX_set_verify(c, SSL_VERIFY_PEER, NULL);
    if (SSL_CTX_set_default_verify_paths(c) != 1) {
        SSL_CTX_free(c);
        return NULL;
    }
    return c;
}

static int dial(const char *host, int port)
{
    struct addrinfo hints, *res = NULL;
    char portstr[8];
    int fd = -1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (getaddrinfo(host, portstr, &hints, &res) != 0) return -1;
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd >= 0 && connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
        close(fd); fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

ssize_t tls_request(const char *host, int port,
                    const char *req_buf, size_t req_len,
                    char *resp_buf, size_t resp_cap)
{
    SSL_CTX *ctx;
    SSL     *ssl  = NULL;
    int      fd   = -1;
    ssize_t  out  = -1;
    size_t   off  = 0;
    int      w;

    if (!host || !req_buf || !resp_buf || resp_cap == 0) return -1;
    ctx = make_ctx();
    if (!ctx) return -1;
    fd  = dial(host, port);
    if (fd < 0) { SSL_CTX_free(ctx); return -1; }
    ssl = SSL_new(ctx);
    if (!ssl) goto done;
    if (SSL_set_tlsext_host_name(ssl, host) != 1) goto done;
    if (SSL_set1_host(ssl, host) != 1) goto done;
    if (SSL_set_fd(ssl, fd) != 1) goto done;
    if (SSL_connect(ssl) != 1) goto done;
    if (SSL_get_verify_result(ssl) != X509_V_OK) goto done;
    for (off = 0; off < req_len; ) {
        w = SSL_write(ssl, req_buf + off, (int)(req_len - off));
        if (w <= 0) goto done;
        off += (size_t)w;
    }
    for (off = 0; off < resp_cap - 1; ) {
        int r = SSL_read(ssl, resp_buf + off, (int)(resp_cap - 1 - off));
        if (r <= 0) break;
        off += (size_t)r;
    }
    resp_buf[off] = '\0';
    out = (ssize_t)off;
done:
    if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
    if (fd >= 0) close(fd);
    SSL_CTX_free(ctx);
    return out;
}
