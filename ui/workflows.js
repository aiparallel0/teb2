/* teb2 workflows + measure views. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
/* workflows with 2s polling until complete */
window.load_workflows = function () {};
window.createRun = function () {
    var gid = document.getElementById("run-goal").value.trim();
    if (!gid) { teb.err("Goal ID required"); return; }
    teb.api("POST", "/runs", { goal_id: parseInt(gid, 10) }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not start run")); return; }
        teb.err(""); teb.info("Run started");
        var rid = r.run_id || (r.run ? r.run.id : r.id);
        window.pollRun(rid, 0);
    });
};
window.pollRun = function (id, attempts) {
    teb.api("GET", "/runs/" + id).then(function (r) {
        var el = document.getElementById("run-status");
        if (r.error) { teb.empty("run-status", teb.errmsg(r, "Could not load run")); return; }
        var run = r.run || r;
        var steps = run.steps || [];
        var stepsHtml = steps.map(function (s) {
            return '<div class="meta">Step ' + s.id + ': '
                + E(s.agent) + ' \u2014 ' + E(s.status) + '</div>';
        }).join("");
        var cancelBtn = (run.status === "running")
            ? ' <button class="btn btn-red" onclick="cancelRun('
              + run.id + ')">Cancel</button>' : '';
        el.innerHTML = '<div class="card">'
            + '<span class="title">Run #' + run.id + '</span>'
            + ' <span class="status status-' + E(run.status || "running")
            + '">' + E(run.status || "running") + '</span>' + cancelBtn
            + '<div class="meta">Goal ' + run.goal_id + '</div>'
            + stepsHtml + '</div>';
        if (run.status === "running" && attempts < 60) {
            setTimeout(function () { window.pollRun(id, attempts + 1); }, 2000);
        }
    });
};
window.cancelRun = function (id) {
    if (!confirm("Cancel run #" + id + "?")) return;
    teb.api("POST", "/runs/" + id + "/cancel", {}).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Cancel failed")); return; }
        teb.err(""); teb.info("Run " + id + " cancellation requested");
        window.pollRun(id, 0);
    });
};
window.checkRun = function (id) { window.pollRun(id, 0); };
/* outcomes & learnings */
window.load_measure = function () {};
window.storeOutcome = function () {
    var tid = document.getElementById("outcome-task").value.trim(),
        res = document.getElementById("outcome-result").value.trim();
    if (!tid || !res) { teb.err("Task ID and result required"); return; }
    teb.api("POST", "/outcomes", {
        task_id: parseInt(tid, 10), result: res
    }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Store failed")); return; }
        teb.err(""); teb.info("Outcome stored (ID " + r.id + ")");
    });
};
window.storeLearning = function () {
    var gid = document.getElementById("learn-goal").value.trim(),
        ins = document.getElementById("learn-insight").value.trim();
    if (!gid || !ins) { teb.err("Goal ID and insight required"); return; }
    teb.api("POST", "/learnings", {
        goal_id: parseInt(gid, 10), insight: ins
    }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Store failed")); return; }
        teb.err(""); teb.info("Learning stored (ID " + r.id + ")");
    });
};
})();
