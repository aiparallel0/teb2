/* teb2 tasks view — CRUD, status, execute. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
function renderTask(t) {
    var s = t.status || "pending";
    return '<div class="card">'
        + '<span class="title">' + E(t.title) + '</span>'
        + ' <span class="status status-' + E(s) + '">' + E(s) + '</span>'
        + '<div class="meta">ID ' + t.id + ' | Goal ' + t.goal_id
        + (t.agent ? ' | ' + E(t.agent) : '') + '</div>'
        + (t.description ? '<div class="desc">' + E(t.description) + '</div>' : '')
        + '<div style="margin-top:.4rem">'
        + '<button class="btn btn-blue" onclick="taskStatus(' + t.id + ')">Status</button> '
        + '<button class="btn btn-green" onclick="taskExec(' + t.id + ')">Execute</button> '
        + '<button class="btn btn-gray" onclick="taskDone(' + t.id + ')">Mark Done</button>'
        + '</div></div>';
}
window.load_tasks = function () {
    var gid = document.getElementById("task-filter-goal").value;
    var url = gid ? "/tasks/goal/" + gid + "?limit=32" : "/tasks/goal/1?limit=32";
    teb.api("GET", url).then(function (r) {
        var el = document.getElementById("task-list");
        if (r.error) { teb.empty("task-list", r.error); return; }
        var rows = r.tasks || r.rows || [];
        if (!Array.isArray(rows)) rows = [];
        if (rows.length === 0) { teb.empty("task-list", "No tasks found"); return; }
        el.innerHTML = rows.map(renderTask).join("");
    });
};
window.createTask = function () {
    var t = document.getElementById("task-title").value;
    var g = document.getElementById("task-goal-id").value;
    var d = document.getElementById("task-desc").value;
    if (!t || !g) { teb.err("Title and Goal ID required"); return; }
    teb.api("POST", "/tasks", {
        title: t, goal_id: parseInt(g, 10), description: d
    }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        document.getElementById("task-title").value = "";
        document.getElementById("task-desc").value = "";
        teb.err(""); window.load_tasks();
    });
};
window.taskStatus = function (id) {
    teb.api("GET", "/tasks/" + id).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        var t = r.task || r;
        var el = document.getElementById("task-detail");
        el.innerHTML = '<div class="card"><span class="title">' + E(t.title) + '</span>'
            + '<div class="desc">' + E(t.description || "") + '</div>'
            + '<div class="meta">Status: ' + E(t.status) + ' | Agent: '
            + E(t.agent || "none") + '</div></div>';
    });
};
window.taskExec = function (id) {
    teb.api("POST", "/tasks/" + id, {}).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err(""); window.load_tasks();
    });
};
window.taskDone = function (id) {
    teb.api("PUT", "/tasks/" + id, { status: "done" }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err(""); window.load_tasks();
    });
};
})();
