/* teb2 core — api, auth, navigation. ≤166 lines */
(function () {
"use strict";
var BASE = window.location.pathname.replace(/\/[^/]*$/, "");
var T = localStorage.getItem("teb2_token") || "";
var EM = localStorage.getItem("teb2_email") || "";
window.teb = {
    base: BASE,
    token: function (v) { if (v !== undefined) { T = v; localStorage.setItem("teb2_token", v); } return T; },
    email: function (v) { if (v !== undefined) { EM = v; localStorage.setItem("teb2_email", v); } return EM; },
    auth_headers: function () { return T ? { "Authorization": "Bearer " + T } : {}; },
    api: function (m, p, b) {
        var o = { method: m, headers: { "Content-Type": "application/json" } };
        if (T) o.headers["Authorization"] = "Bearer " + T;
        if (b) o.body = JSON.stringify(b);
        return fetch(BASE + p, o).then(function (r) {
            return r.json().then(function (j) {
                j._status = r.status;
                /* Expired or invalid token: clear it so the login form
                 * reappears instead of repeating 401 on every page load. */
                if (r.status === 401 && T) { teb.token(""); teb.email(""); refreshAuthUI(); }
                return j;
            }).catch(function () { return { _status: r.status, error: "bad_response" }; });
        }).catch(function () { return { _status: 0, error: "network_error" }; });
    },
    /* Normalize list responses: backend returns {items:[...]}, {rows:[...]},
       {data:[...]}, {top:[...]}, {hits:[...]} etc. Returns a plain array. */
    list: function (r) {
        var k, keys = ["items", "rows", "data", "hits", "top",
                       "messages", "entries", "goals", "tasks", "names"];
        for (k = 0; k < keys.length; k++) {
            if (Array.isArray(r[keys[k]])) return r[keys[k]];
        }
        return [];
    },
    err: function (msg) {
        var el = document.getElementById("error-bar");
        if (!el) return;
        el.textContent = msg || "";
        el.className = "bar-error";
        el.style.display = msg ? "block" : "none";
    },
    info: function (msg) {
        var el = document.getElementById("error-bar");
        if (!el) return;
        el.textContent = msg || "";
        el.className = "bar-info";
        el.style.display = msg ? "block" : "none";
        if (msg) setTimeout(function () {
            if (el.textContent === msg) { el.textContent = ""; el.style.display = "none"; }
        }, 3000);
    },
    /* Friendly mapper for HTTP/backend error responses. */
    errmsg: function (r, fallback) {
        if (!r) return fallback || "network_error";
        if (r._status === 401) return "Please log in";
        if (r._status === 403) return "Not allowed";
        if (r._status === 404) return fallback || "Not found";
        if (r._status === 429) return "Rate limited — try again shortly";
        if (r._status === 503 || r._status === 502 || r._status === 504)
            return "Service unavailable — try again in a moment";
        if (r._status === 0) return "Network error — check your connection";
        return r.error || fallback || "Error";
    },
    esc: function (s) {
        var d = document.createElement("div"); d.textContent = (s == null ? "" : String(s)); return d.innerHTML;
    },
    empty: function (id, msg) {
        var el = document.getElementById(id);
        if (el) el.innerHTML = '<div class="empty">' + (msg || "No items") + '</div>';
    }
};
function refreshAuthUI() {
    var logged = !!T;
    var emEl = document.getElementById("email");
    var pwEl = document.getElementById("password");
    var who  = document.getElementById("who");
    if (who) who.textContent = logged ? (EM ? ("\u25CF " + EM) : "\u25CF signed in") : "";
    document.querySelectorAll("[data-auth=out]").forEach(function (el) {
        el.style.display = logged ? "none" : "";
    });
    document.querySelectorAll("[data-auth=in]").forEach(function (el) {
        el.style.display = logged ? "" : "none";
    });
    if (!logged) { if (emEl) emEl.value = ""; if (pwEl) pwEl.value = ""; }
}
window.teb.refreshAuthUI = refreshAuthUI;
/* auth - guarded against double-submit. The previous code allowed a click
 * storm during a slow network round-trip, producing long streams of
 * identical 401s in the console before any response had returned. */
var AUTH_BUSY = false;
function authBusy(v) {
    AUTH_BUSY = v;
    ["btn-login", "btn-register", "btn-forgot"].forEach(function (id) {
        var b = document.getElementById(id);
        if (b) b.disabled = v;
    });
}
function signIn(e, p, onFail) {
    return teb.api("POST", "/auth/login", { email: e, password: p })
        .then(function (r) {
            if (r.error) { if (onFail) onFail(r); return false; }
            teb.token(r.token || ""); teb.email(e); teb.err(""); refreshAuthUI();
            teb.info("Signed in as " + e);
            loadAll();
            return true;
        });
}
window.doLogin = function () {
    if (AUTH_BUSY) return;
    var e = document.getElementById("email").value.trim();
    var p = document.getElementById("password").value;
    if (!e || !p) { teb.err("Email and password required"); return; }
    authBusy(true);
    signIn(e, p, function (r) {
        if (r._status === 401)
            teb.err("Wrong email or password. Click Forgot? to reset.");
        else teb.err(teb.errmsg(r, "Login failed"));
    }).then(function () { authBusy(false); });
};
window.doRegister = function () {
    if (AUTH_BUSY) return;
    var e = document.getElementById("email").value.trim();
    var p = document.getElementById("password").value;
    if (!e || !p) { teb.err("Email and password required"); return; }
    if (p.length < 8) { teb.err("Password must be at least 8 characters"); return; }
    authBusy(true);
    teb.api("POST", "/auth/register", { email: e, password: p }).then(function (r) {
        if (!r.error) {
            teb.err(""); teb.info("Account created — signing in\u2026");
            return signIn(e, p, function (r2) {
                teb.err(teb.errmsg(r2, "Sign-in after register failed"));
            });
        }
        /* 409: account already exists. Try the same credentials so a
         * user who forgot they already registered is signed in instead
         * of bouncing between error messages. */
        if (r._status === 409) {
            teb.info("Account already exists — signing in\u2026");
            return signIn(e, p, function (r2) {
                if (r2._status === 401)
                    teb.err("Account exists — wrong password. Click Forgot? to reset.");
                else teb.err(teb.errmsg(r2, "Sign-in failed"));
            });
        }
        teb.err(teb.errmsg(r, "Registration failed"));
    }).then(function () { authBusy(false); });
};
window.doForgot = function () {
    if (AUTH_BUSY) return;
    var e = document.getElementById("email").value.trim();
    var np = document.getElementById("password").value;
    if (!e) { teb.err("Enter your email, then click Forgot?"); return; }
    if (!np || np.length < 8) {
        teb.err("Type the NEW password (min 8 chars) in the password field, then click Forgot?");
        return;
    }
    authBusy(true);
    teb.api("POST", "/auth/forgot", { email: e }).then(function (r) {
        authBusy(false);
        if (r.error) { teb.err(teb.errmsg(r, "Reset request failed")); return; }
        var t = window.prompt("A reset token was emailed to " + e +
                              ".\nPaste the token here to apply the new password:");
        if (!t) return;
        teb.api("POST", "/auth/reset", { token: t.trim(), password: np }).then(function (r2) {
            if (r2.error) { teb.err(teb.errmsg(r2, "Reset failed")); return; }
            teb.err(""); teb.info("Password updated — you can sign in now");
        });
    });
};
window.doLogout = function () {
    teb.token(""); teb.email(""); localStorage.removeItem("teb2_token");
    localStorage.removeItem("teb2_email");
    document.querySelectorAll(".view").forEach(function (v) {
        v.querySelectorAll("[data-list]").forEach(function (el) { el.innerHTML = ""; });
    });
    teb.err(""); refreshAuthUI(); teb.info("Signed out");
};
/* navigation */
window.showView = function (name) {
    document.querySelectorAll(".view").forEach(function (v) {
        v.classList.toggle("active", v.id === "v-" + name);
    });
    document.querySelectorAll("nav button").forEach(function (b) {
        b.classList.toggle("active", b.dataset.view === name);
    });
    if (!T) { teb.err("Log in to use this feature"); return; }
    var fn = window["load_" + name];
    if (fn) fn();
};
/* health */
function checkHealth() {
    fetch(BASE + "/healthz").then(function (r) { return r.json(); })
        .then(function (j) {
            document.getElementById("health").textContent =
                j.status === "ok" ? "\u25CF online" : "\u25CB offline";
        })
        .catch(function () {
            document.getElementById("health").textContent = "\u25CB offline";
        });
}
function loadAll() {
    if (window.load_goals) window.load_goals();
}
checkHealth();
refreshAuthUI();
if (T) loadAll();
window.showView("goals");
})();
