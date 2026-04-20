/* teb2 goals view — CRUD for goals with inline decomposition. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
function renderGoal(g) {
    var s = g.status || "pending";
    return '<div class="card" onclick="viewGoal(' + g.id + ')" '
        + 'style="cursor:pointer">'
        + '<span class="title">' + E(g.title) + '</span>'
        + ' <span class="status status-' + E(s) + '">' + E(s) + '</span>'
        + '<div class="meta">ID ' + g.id + '</div>'
        + (g.description ? '<div class="desc">' + E(g.description) + '</div>' : '')
        + '</div>';
}
window.load_goals = function () {
    if (!teb.token()) { teb.empty("goal-list", "Log in to see your goals"); return; }
    teb.api("GET", "/goals").then(function (r) {
        if (r.error) { teb.empty("goal-list", teb.errmsg(r, "Could not load goals")); return; }
        var rows = teb.list(r);
        if (rows.length === 0) { teb.empty("goal-list", "No goals yet — add one above"); return; }
        document.getElementById("goal-list").innerHTML = rows.map(renderGoal).join("");
    });
};
window.createGoal = function () {
    var t = document.getElementById("goal-title").value.trim();
    var d = document.getElementById("goal-desc").value;
    if (!t) { teb.err("Goal title required"); return; }
    teb.api("POST", "/goals", { title: t, description: d }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not create goal")); return; }
        document.getElementById("goal-title").value = "";
        document.getElementById("goal-desc").value = "";
        teb.err(""); teb.info("Goal created"); window.load_goals();
    });
};
window.viewGoal = function (id) {
    teb.api("GET", "/goal/" + id).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not load goal")); return; }
        var g = r.goal || r;
        var el = document.getElementById("goal-detail");
        el.innerHTML = '<div class="card">'
            + '<span class="title">' + E(g.title) + '</span>'
            + ' <span class="status status-' + E(g.status || "pending")
            + '">' + E(g.status || "pending") + '</span>'
            + (g.description ? '<div class="desc">' + E(g.description) + '</div>' : '')
            + '<div class="meta">ID ' + g.id + (g.created_at
                ? ' | Created ' + new Date((g.created_at || 0) * 1000).toLocaleDateString()
                : '') + '</div>'
            + '</div>'
            + '<button class="btn btn-blue" onclick="decomposeGoal('
            + g.id + ')">Decompose with AI</button>'
            + ' <button class="btn btn-gray" onclick="loadGoalTasks('
            + g.id + ')">Show Tasks</button>';
        document.getElementById("goal-tasks").innerHTML = "";
    });
};
window.decomposeGoal = function (id) {
    var el = document.getElementById("goal-tasks");
    el.innerHTML = '<div class="empty">Decomposing with AI\u2026</div>';
    teb.api("POST", "/goal/" + id).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Decomposition failed")); el.innerHTML = ""; return; }
        teb.err(""); teb.info("Created task #" + (r.task_id || "?"));
        window.loadGoalTasks(id);
    });
};
window.loadGoalTasks = function (id) {
    teb.api("GET", "/tasks/goal/" + id + "?limit=32").then(function (r) {
        var el = document.getElementById("goal-tasks");
        if (r.error) { el.innerHTML = '<div class="empty">' + E(teb.errmsg(r, "Could not load tasks")) + '</div>'; return; }
        var rows = teb.list(r);
        if (rows.length === 0) {
            el.innerHTML = '<div class="empty">No tasks yet — try Decompose with AI</div>';
            return;
        }
        el.innerHTML = '<h2>Tasks for Goal ' + id + '</h2>'
            + rows.map(function (t) {
                var s = t.status || "pending";
                return '<div class="card">'
                    + '<span class="title">' + E(t.title) + '</span>'
                    + ' <span class="status status-' + E(s) + '">' + E(s) + '</span>'
                    + '<div class="meta">Task ' + t.id
                    + (t.agent ? ' | Agent: ' + E(t.agent) : '') + '</div>'
                    + (t.description ? '<div class="desc">' + E(t.description) + '</div>' : '')
                    + '</div>';
            }).join("");
    });
};
})();
