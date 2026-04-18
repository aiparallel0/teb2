/* teb2 core — api, auth, navigation. ≤166 lines */
(function () {
"use strict";
var BASE = window.location.pathname.replace(/\/[^/]*$/, "");
var T = localStorage.getItem("teb2_token") || "";
window.teb = {
    base: BASE,
    token: function (v) { if (v !== undefined) { T = v; localStorage.setItem("teb2_token", v); } return T; },
    api: function (m, p, b) {
        var o = { method: m, headers: { "Content-Type": "application/json" } };
        if (T) o.headers["Authorization"] = "Bearer " + T;
        if (b) o.body = JSON.stringify(b);
        return fetch(BASE + p, o).then(function (r) {
            return r.json().then(function (j) { j._status = r.status; return j; });
        });
    },
    err: function (msg) {
        var el = document.getElementById("error-bar");
        el.textContent = msg; el.style.display = msg ? "block" : "none";
    },
    esc: function (s) {
        var d = document.createElement("div"); d.textContent = s || ""; return d.innerHTML;
    },
    empty: function (id, msg) {
        document.getElementById(id).innerHTML = '<div class="empty">' + (msg || "No items") + '</div>';
    }
};
/* auth */
window.doLogin = function () {
    var e = document.getElementById("email").value;
    var p = document.getElementById("password").value;
    teb.api("POST", "/auth/login", { email: e, password: p }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.token(r.token || ""); teb.err(""); loadAll();
    });
};
window.doRegister = function () {
    var e = document.getElementById("email").value;
    var p = document.getElementById("password").value;
    teb.api("POST", "/auth/register", { email: e, password: p }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err(""); window.doLogin();
    });
};
window.doLogout = function () {
    teb.token(""); localStorage.removeItem("teb2_token");
    document.querySelectorAll(".view").forEach(function (v) { v.innerHTML = ""; });
    teb.err("");
};
/* navigation */
window.showView = function (name) {
    document.querySelectorAll(".view").forEach(function (v) {
        v.classList.toggle("active", v.id === "v-" + name);
    });
    document.querySelectorAll("nav button").forEach(function (b) {
        b.classList.toggle("active", b.dataset.view === name);
    });
    var fn = window["load_" + name];
    if (fn) fn();
};
/* health */
function checkHealth() {
    fetch(BASE + "/healthz").then(function (r) { return r.json(); })
        .then(function (j) {
            document.getElementById("health").textContent =
                j.status === "ok" ? "\u25CF connected" : "\u25CB disconnected";
        })
        .catch(function () {
            document.getElementById("health").textContent = "\u25CB disconnected";
        });
}
function loadAll() {
    if (window.load_goals) window.load_goals();
}
checkHealth();
if (T) loadAll();
window.showView("goals");
})();
