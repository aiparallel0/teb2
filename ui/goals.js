/* teb2 goals view — CRUD for goals with inline decomposition. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
function renderGoal(g) {
    var s = g.status || "pending";
    return '<div class="card" onclick="viewGoal(' + g.id + ')">'
        + '<span class="title">' + E(g.title) + '</span>'
        + ' <span class="status status-' + E(s) + '">' + E(s) + '</span>'
        + '<div class="meta">ID ' + g.id + '</div>'
        + (g.description ? '<div class="desc">' + E(g.description) + '</div>' : '')
        + '</div>';
}
window.load_goals = function () {
    teb.api("GET", "/goals").then(function (r) {
        var el = document.getElementById("goal-list");
        if (r.error) { teb.empty("goal-list", r.error); return; }
        var rows = r.goals || r.rows || [];
        if (!Array.isArray(rows)) rows = [];
        if (rows.length === 0) { teb.empty("goal-list", "No goals yet"); return; }
        el.innerHTML = rows.map(renderGoal).join("");
    });
};
window.createGoal = function () {
    var t = document.getElementById("goal-title").value;
    var d = document.getElementById("goal-desc").value;
    if (!t) return;
    teb.api("POST", "/goals", { title: t, description: d }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        document.getElementById("goal-title").value = "";
        document.getElementById("goal-desc").value = "";
        teb.err(""); window.load_goals();
    });
};
window.viewGoal = function (id) {
    teb.api("GET", "/goal/" + id).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        var g = r.goal || r;
        var el = document.getElementById("goal-detail");
        el.innerHTML = '<div class="card">'
            + '<span class="title">' + E(g.title) + '</span>'
            + ' <span class="status status-' + E(g.status || "pending")
            + '">' + E(g.status || "pending") + '</span>'
            + '<div class="desc">' + E(g.description || "") + '</div>'
            + '<div class="meta">ID ' + g.id + ' | Created '
            + new Date((g.created_at || 0) * 1000).toLocaleDateString() + '</div>'
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
        if (r.error) { teb.err(r.error); el.innerHTML = ""; return; }
        teb.err("");
        /* Show task_id from response if available, then load full task list */
        if (r.task_id) {
            el.innerHTML = '<div class="empty">Created task #' + r.task_id
                + ' \u2014 loading all tasks\u2026</div>';
        }
        window.loadGoalTasks(id);
    });
};
window.loadGoalTasks = function (id) {
    teb.api("GET", "/tasks/goal/" + id + "?limit=32").then(function (r) {
        var el = document.getElementById("goal-tasks");
        if (r.error) { el.innerHTML = ""; return; }
        var rows = r.tasks || r.rows || [];
        if (!Array.isArray(rows)) rows = [];
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
