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
        if (r.error) { teb.empty("budget-list", r.error); return; }
        var b = r.budget || r;
        if (!b || !b.id) { teb.empty("budget-list", "No budget set"); return; }
        var pct = b.limit_cents > 0
            ? Math.round((b.spent_cents / b.limit_cents) * 100) : 0;
        el.innerHTML = '<div class="card">'
            + '<span class="title">Budget</span>'
            + '<div class="meta">Limit: $' + (b.limit_cents / 100).toFixed(2)
            + ' | Spent: $' + (b.spent_cents / 100).toFixed(2)
            + ' (' + pct + '%)</div>'
            + '<div style="background:#333;border-radius:3px;height:8px;margin-top:.5rem">'
            + '<div style="background:' + (pct > 90 ? '#dc2626' : '#16a34a')
            + ';width:' + Math.min(pct, 100)
            + '%;height:100%;border-radius:3px"></div></div>'
            + '</div>';
    });
}
function loadSchedules() {
    teb.api("GET", "/schedules/1").then(function (r) {
        var el = document.getElementById("sched-list");
        if (r.error) { teb.empty("sched-list", r.error); return; }
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
    var a = document.getElementById("budget-amount").value;
    if (!a) return;
    var cents = Math.round(parseFloat(a) * 100);
    teb.api("POST", "/budgets", { limit_cents: cents }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        document.getElementById("budget-amount").value = "";
        teb.err(""); loadBudgets();
    });
};
window.recordSpend = function () {
    var a = document.getElementById("spend-amount").value;
    if (!a) return;
    var cents = Math.round(parseFloat(a) * 100);
    teb.api("POST", "/spending", { amount_cents: cents }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        document.getElementById("spend-amount").value = "";
        teb.err(""); loadBudgets();
    });
};
window.createSchedule = function () {
    var tid = document.getElementById("sched-task").value;
    var dt = document.getElementById("sched-time").value;
    if (!tid || !dt) return;
    var ts = Math.floor(new Date(dt).getTime() / 1000);
    teb.api("POST", "/schedules", { task_id: parseInt(tid, 10), run_at: ts })
        .then(function (r) {
            if (r.error) { teb.err(r.error); return; }
            teb.err(""); loadSchedules();
        });
};
})();
