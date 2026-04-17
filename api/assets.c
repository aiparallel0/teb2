#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"

static void detect_mime(const char *fn, char *out, size_t outsz)
{
    const char *dot = strrchr(fn, '.');
    if (!dot) { snprintf(out, outsz, "application/octet-stream"); return; }
    if (strcmp(dot, ".png") == 0)  { snprintf(out, outsz, "image/png"); return; }
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) {
        snprintf(out, outsz, "image/jpeg"); return; }
    if (strcmp(dot, ".pdf") == 0)  { snprintf(out, outsz, "application/pdf"); return; }
    snprintf(out, outsz, "application/octet-stream");
}

/* Strip any path-separator or dot-dot segments from a user-supplied
 * filename so an attacker cannot escape the asset directory. Only
 * alnum, dot, underscore, dash are preserved. */
static void sanitize_filename(const char *in, char *out, size_t outsz)
{
    size_t i, o = 0;
    if (outsz == 0) return;
    for (i = 0; in[i] && o + 1 < outsz; i++) {
        char c = in[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (ok) out[o++] = c;
    }
    if (o == 0) {
        out[o++] = 'f';
    } else if (out[0] == '.') {
        out[o++] = 'f';
    }
    out[o] = '\0';
}

HttpResp handle_asset_upload(HttpReq req, Ctx *ctx)
{
    AssetQuery q;
    AssetResult r;
    char buf[256], fn[256], clean[256], uid[64];
    const char *fnp;
    int fd;
    ssize_t nw;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(uid, sizeof(uid), "%lld", (long long)ctx->user->user_id);
    snprintf(q.user_id, sizeof(q.user_id), "%s", uid);
    fnp = strstr(req.query, "filename=");
    if (fnp && (fnp == req.query || fnp[-1] == '&')) {
        fnp += 9;
        snprintf(fn, sizeof(fn), "%.*s", (int)strcspn(fnp, "& "), fnp);
    } else {
        snprintf(fn, sizeof(fn), "upload");
    }
    sanitize_filename(fn, clean, sizeof(clean));
    snprintf(q.filename, sizeof(q.filename), "%s", clean);
    detect_mime(clean, q.mime_type, sizeof(q.mime_type));
    q.size_bytes = (int64_t)req.body_len;
    (void)mkdir("/tmp/teb_assets", 0700);
    if (req.body_len >= sizeof(req.body) - 1) return json_error(413, "payload_too_large");
    snprintf(q.path, sizeof(q.path), "/tmp/teb_assets/%s_%lld_%s",
             uid, (long long)time(NULL), clean);
    fd = open(q.path, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0) return json_error(500, "file_error");
    nw = write(fd, req.body, req.body_len);
    close(fd);
    if (nw < 0) return json_error(500, "write_error");
    r = store_asset(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"filename\":\"%.64s\",\"size_bytes\":%lld}",
             (long long)r.asset.id, clean, (long long)r.asset.size_bytes);
    return json_ok(buf);
}

HttpResp handle_asset_get(HttpReq req, Ctx *ctx)
{
    AssetQuery q;
    AssetResult r;
    HttpResp resp;
    char uid[64];
    int fd;
    ssize_t nr;
    const char *idstr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");
    r = fetch_asset(ctx->db, q);
    if (r.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    /* Tenant check: user may only fetch their own assets unless admin. */
    snprintf(uid, sizeof(uid), "%lld", (long long)ctx->user->user_id);
    if (ctx->user->role != ROLE_ADMIN &&
        strcmp(r.asset.user_id, uid) != 0)
        return json_error(403, "forbidden");
    memset(&resp, 0, sizeof(resp));
    fd = open(r.asset.path, O_RDONLY);
    if (fd < 0) return json_error(404, "file_missing");
    nr = read(fd, resp.body, sizeof(resp.body) - 1);
    close(fd);
    if (nr < 0) return json_error(500, "read_error");
    resp.body_len = (size_t)nr;
    resp.status = 200;
    snprintf(resp.content_type, sizeof(resp.content_type), "%.63s", r.asset.mime_type);
    return resp;
}
