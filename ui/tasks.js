/* teb2 tasks view — CRUD, status, execute. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
function renderTask(t) {
    var s = t.status || "pending";
    return '<div class="card">'
        + '<span class="title">' + E(t.title) + '</span>'
        + ' <span class="status status-' + E(s) + '">' + E(s) + '</span>'
        + '<div class="meta">ID ' + t.id
        + (t.goal_id ? ' | Goal ' + t.goal_id : '')
        + (t.agent ? ' | ' + E(t.agent) : '') + '</div>'
        + (t.description ? '<div class="desc">' + E(t.description) + '</div>' : '')
        + '<div style="margin-top:.4rem">'
        + '<button class="btn btn-blue" onclick="taskStatus(' + t.id + ')">Status</button> '
        + '<button class="btn btn-green" onclick="taskExec(' + t.id + ')">Execute</button> '
        + '<button class="btn btn-gray" onclick="taskDone(' + t.id + ')">Mark Done</button>'
        + '</div></div>';
}
window.load_tasks = function () {
    var gid = document.getElementById("task-filter-goal").value.trim();
    if (!gid) {
        teb.empty("task-list", "Enter a Goal ID above and click Filter to list tasks");
        return;
    }
    teb.api("GET", "/tasks/goal/" + gid + "?limit=32").then(function (r) {
        if (r.error) { teb.empty("task-list", teb.errmsg(r, "Could not load tasks")); return; }
        var rows = teb.list(r);
        if (rows.length === 0) { teb.empty("task-list", "No tasks for goal " + gid); return; }
        document.getElementById("task-list").innerHTML = rows.map(renderTask).join("");
    });
};
window.createTask = function () {
    var t = document.getElementById("task-title").value.trim();
    var g = document.getElementById("task-goal-id").value.trim();
    var d = document.getElementById("task-desc").value;
    if (!t || !g) { teb.err("Task title and Goal ID required"); return; }
    teb.api("POST", "/tasks", {
        title: t, goal_id: parseInt(g, 10), description: d
    }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not create task")); return; }
        document.getElementById("task-title").value = "";
        document.getElementById("task-desc").value = "";
        /* Auto-fill filter with goal id so the new task is visible */
        document.getElementById("task-filter-goal").value = g;
        teb.err(""); teb.info("Task created");
        window.load_tasks();
    });
};
window.taskStatus = function (id) {
    teb.api("GET", "/tasks/" + id).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not load task")); return; }
        var t = r.task || r;
        var el = document.getElementById("task-detail");
        el.innerHTML = '<div class="card"><span class="title">' + E(t.title || ("Task " + t.id)) + '</span>'
            + (t.description ? '<div class="desc">' + E(t.description) + '</div>' : '')
            + '<div class="meta">ID ' + t.id
            + ' | Status: ' + E(t.status || "pending")
            + (t.goal_id ? ' | Goal: ' + t.goal_id : '')
            + ' | Agent: ' + E(t.agent || "none") + '</div></div>';
    });
};
window.taskExec = function (id) {
    teb.api("POST", "/tasks/" + id, {}).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Execution failed")); return; }
        teb.err(""); teb.info("Task executing"); window.load_tasks();
    });
};
window.taskDone = function (id) {
    teb.api("PUT", "/tasks/" + id, { status: "done" }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Update failed")); return; }
        teb.err(""); teb.info("Task marked done"); window.load_tasks();
    });
};
})();
