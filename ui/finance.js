/* teb2 finance — budgets, spending, schedules. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_finance = function () {
    loadBudgets();
    loadSchedules();
};
function loadBudgets() {
    teb.api("GET", "/budgets").then(function (r) {
        var el = document.getElementById("budget-list");
        if (r.error) {
            teb.empty("budget-list", r._status === 404
                ? "No budget set — set one above"
                : teb.errmsg(r, "Could not load budget"));
            return;
        }
        var b = r.budget || r;
        if (!b || !b.id) { teb.empty("budget-list", "No budget set"); return; }
        var pct = b.limit_cents > 0
            ? Math.round((b.spent_cents / b.limit_cents) * 100) : 0;
        el.innerHTML = '<div class="card">'
            + '<span class="title">Budget</span>'
            + '<div class="meta">Limit: $' + (b.limit_cents / 100).toFixed(2)
            + ' | Spent: $' + (b.spent_cents / 100).toFixed(2)
            + ' (' + pct + '%)</div>'
            + '<div class="progress">'
            + '<div class="progress-fill" style="background:'
            + (pct > 90 ? '#dc2626' : '#16a34a')
            + ';width:' + Math.min(pct, 100) + '%"></div></div>'
            + '</div>';
    });
}
function loadSchedules() {
    var tid = document.getElementById("sched-task").value.trim() || "1";
    teb.api("GET", "/schedules/" + tid).then(function (r) {
        var el = document.getElementById("sched-list");
        if (r.error) {
            teb.empty("sched-list", r._status === 404
                ? "No schedule for task " + tid
                : teb.errmsg(r, "Could not load schedules"));
            return;
        }
        var s = r.entry || r;
        if (!s || !s.id) { teb.empty("sched-list", "No schedules"); return; }
        el.innerHTML = '<div class="card">'
            + '<span class="title">Schedule #' + s.id + '</span>'
            + '<div class="meta">Task: ' + s.task_id
            + ' | Run at: ' + new Date((s.run_at || 0) * 1000).toLocaleString()
            + '</div></div>';
    });
}
window.createBudget = function () {
    var a = document.getElementById("budget-amount").value.trim();
    if (!a) { teb.err("Budget amount required"); return; }
    var cents = Math.round(parseFloat(a) * 100);
    if (cents <= 0) { teb.err("Budget must be positive"); return; }
    teb.api("POST", "/budgets", { limit_cents: cents }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not set budget")); return; }
        document.getElementById("budget-amount").value = "";
        teb.err(""); teb.info("Budget set to $" + a); loadBudgets();
    });
};
window.recordSpend = function () {
    var a = document.getElementById("spend-amount").value.trim();
    if (!a) { teb.err("Spend amount required"); return; }
    var cents = Math.round(parseFloat(a) * 100);
    if (cents <= 0) { teb.err("Amount must be positive"); return; }
    teb.api("POST", "/spending", { amount_cents: cents }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not record spending")); return; }
        document.getElementById("spend-amount").value = "";
        teb.err(""); teb.info("Recorded $" + a + " spending"); loadBudgets();
    });
};
window.createSchedule = function () {
    var tid = document.getElementById("sched-task").value.trim();
    var dt = document.getElementById("sched-time").value;
    if (!tid || !dt) { teb.err("Task ID and time required"); return; }
    var ts = Math.floor(new Date(dt).getTime() / 1000);
    if (!ts || isNaN(ts)) { teb.err("Invalid date/time"); return; }
    teb.api("POST", "/schedules", { task_id: parseInt(tid, 10), run_at: ts })
        .then(function (r) {
            if (r.error) { teb.err(teb.errmsg(r, "Could not schedule")); return; }
            teb.err(""); teb.info("Scheduled task " + tid); loadSchedules();
        });
};
})();
