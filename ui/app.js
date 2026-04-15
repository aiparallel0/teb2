/* teb2 frontend — vanilla JS, no frameworks, no build step */
(function () {
    "use strict";

    var BASE = "";
    var token = localStorage.getItem("teb2_token") || "";

    function api(method, path, body) {
        var opts = {
            method: method,
            headers: { "Content-Type": "application/json" }
        };
        if (token) opts.headers["Authorization"] = "Bearer " + token;
        if (body) opts.body = JSON.stringify(body);
        return fetch(BASE + path, opts).then(function (r) {
            return r.json().then(function (j) {
                j._status = r.status;
                return j;
            });
        });
    }

    function showError(msg) {
        var el = document.getElementById("error-bar");
        el.textContent = msg;
        el.style.display = msg ? "block" : "none";
    }

    function esc(s) {
        var d = document.createElement("div");
        d.textContent = s || "";
        return d.innerHTML;
    }

    /* auth */
    window.doLogin = function () {
        var e = document.getElementById("email").value;
        var p = document.getElementById("password").value;
        api("POST", "/auth/login", { email: e, password: p }).then(function (r) {
            if (r.error) { showError(r.error); return; }
            token = r.token || "";
            localStorage.setItem("teb2_token", token);
            showError("");
            loadAll();
        });
    };

    window.doRegister = function () {
        var e = document.getElementById("email").value;
        var p = document.getElementById("password").value;
        api("POST", "/auth/register", { email: e, password: p }).then(function (r) {
            if (r.error) { showError(r.error); return; }
            showError("");
            window.doLogin();
        });
    };

    window.doLogout = function () {
        token = "";
        localStorage.removeItem("teb2_token");
        document.getElementById("goal-list").innerHTML = "";
        document.getElementById("task-list").innerHTML = "";
        showError("");
    };

    /* goals */
    function loadGoals() {
        api("GET", "/goals").then(function (r) {
            var el = document.getElementById("goal-list");
            if (r.error) { el.innerHTML = ""; return; }
            var rows = r.goals || r.rows || [];
            if (!Array.isArray(rows)) rows = [];
            el.innerHTML = rows.map(function (g) {
                return '<div class="card">'
                    + '<span class="title">' + esc(g.title) + '</span>'
                    + ' <span class="status status-'
                    + esc(g.status || "pending") + '">'
                    + esc(g.status || "pending") + '</span>'
                    + '<div class="meta">ID ' + g.id + '</div>'
                    + '</div>';
            }).join("");
        });
    }

    window.createGoal = function () {
        var t = document.getElementById("goal-title").value;
        if (!t) return;
        api("POST", "/goals", { title: t }).then(function (r) {
            if (r.error) { showError(r.error); return; }
            document.getElementById("goal-title").value = "";
            showError("");
            loadGoals();
        });
    };

    /* tasks */
    function loadTasks() {
        api("GET", "/tasks/goal/0?limit=32").then(function (r) {
            var el = document.getElementById("task-list");
            if (r.error) { el.innerHTML = ""; return; }
            var rows = r.tasks || r.rows || [];
            if (!Array.isArray(rows)) rows = [];
            el.innerHTML = rows.map(function (t) {
                return '<div class="card">'
                    + '<span class="title">' + esc(t.title) + '</span>'
                    + ' <span class="status status-'
                    + esc(t.status || "pending") + '">'
                    + esc(t.status || "pending") + '</span>'
                    + '<div class="meta">ID ' + t.id
                    + ' | Goal ' + t.goal_id + '</div>'
                    + '</div>';
            }).join("");
        });
    }

    window.createTask = function () {
        var t = document.getElementById("task-title").value;
        var g = document.getElementById("task-goal").value;
        if (!t) return;
        api("POST", "/tasks", { title: t, goal_id: parseInt(g, 10) || 0 })
            .then(function (r) {
                if (r.error) { showError(r.error); return; }
                document.getElementById("task-title").value = "";
                showError("");
                loadTasks();
            });
    };

    /* health check */
    function checkHealth() {
        fetch(BASE + "/healthz").then(function (r) { return r.json(); })
            .then(function (j) {
                document.getElementById("health").textContent =
                    j.status === "ok" ? "● connected" : "○ disconnected";
            })
            .catch(function () {
                document.getElementById("health").textContent = "○ disconnected";
            });
    }

    function loadAll() { loadGoals(); loadTasks(); }

    checkHealth();
    if (token) loadAll();
})();
