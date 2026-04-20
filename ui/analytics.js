/* teb2 analytics — progress, ROI, time tracking, nudges. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_analytics = function () {
    loadROI(); loadNudge();
};
/* progress snapshots */
window.storeSnapshot = function () {
    var gid = document.getElementById("snap-goal").value.trim();
    var pct = document.getElementById("snap-pct").value.trim();
    if (!gid || !pct) { teb.err("Goal ID and percent required"); return; }
    teb.api("POST", "/analytics/snap", {
        goal_id: parseInt(gid, 10), pct: parseInt(pct, 10)
    }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Snapshot failed")); return; }
        teb.err(""); teb.info("Snapshot stored — " + pct + "%");
        document.getElementById("snap-pct").value = "";
    });
};
/* ROI */
function loadROI() {
    var gid = document.getElementById("roi-goal").value.trim() || "1";
    teb.api("GET", "/roi/" + gid).then(function (r) {
        if (r.error) { teb.empty("roi-detail", teb.errmsg(r, "Could not load ROI")); return; }
        var m = r.roi || r;
        var ratio = m.cost_cents > 0
            ? (((m.value_cents - m.cost_cents) / m.cost_cents) * 100).toFixed(0) + "%"
            : "\u221E";
        document.getElementById("roi-detail").innerHTML = '<div class="grid">'
            + '<div class="stat"><div class="val">$'
            + ((m.value_cents || 0) / 100).toFixed(2)
            + '</div><div class="lbl">Value</div></div>'
            + '<div class="stat"><div class="val">$'
            + ((m.cost_cents || 0) / 100).toFixed(2)
            + '</div><div class="lbl">Cost</div></div>'
            + '<div class="stat"><div class="val">'
            + ratio + '</div><div class="lbl">ROI</div></div></div>';
    });
}
window.loadROI = loadROI;
/* time tracking */
window.logTime = function () {
    var tid = document.getElementById("time-task").value.trim();
    var mins = document.getElementById("time-mins").value.trim();
    if (!tid || !mins) { teb.err("Task ID and minutes required"); return; }
    teb.api("POST", "/analytics/time", {
        task_id: parseInt(tid, 10), minutes: parseInt(mins, 10)
    }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Log failed")); return; }
        teb.err(""); teb.info(mins + " minutes logged");
        document.getElementById("time-mins").value = "";
    });
};
/* nudges */
function loadNudge() {
    teb.api("GET", "/nudge/1").then(function (r) {
        if (r.error) {
            teb.empty("nudge-list", r._status === 404
                ? "No nudges yet — send one below"
                : teb.errmsg(r, "Could not load nudges"));
            return;
        }
        var n = r.nudge || r;
        if (!n || !n.id) { teb.empty("nudge-list", "No nudges yet"); return; }
        document.getElementById("nudge-list").innerHTML = '<div class="card">'
            + '<span class="title">Nudge #' + n.id + '</span>'
            + '<div class="desc">' + E(n.message) + '</div>'
            + (n.created_at ? '<div class="meta">'
                + new Date((n.created_at || 0) * 1000).toLocaleString()
                + '</div>' : '')
            + '</div>';
    });
}
window.sendNudge = function () {
    var msg = document.getElementById("nudge-msg").value.trim();
    if (!msg) { teb.err("Nudge message required"); return; }
    teb.api("POST", "/nudges", { message: msg }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Nudge failed")); return; }
        teb.err(""); teb.info("Nudge sent (#" + r.id + ")");
        document.getElementById("nudge-msg").value = "";
        loadNudge();
    });
};
})();
