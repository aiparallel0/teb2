/* teb2 dashboard — search, gamification, analytics, community, workflows. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_dash = function () {
    loadStreak(); loadLeaderboard();
};
/* search */
window.doSearch = function () {
    var q = document.getElementById("search-q").value;
    if (!q) return;
    teb.api("GET", "/search?q=" + encodeURIComponent(q)).then(function (r) {
        var el = document.getElementById("search-results");
        if (r.error) { teb.empty("search-results", r.error); return; }
        var hits = r.hits || [];
        if (!Array.isArray(hits)) hits = [];
        if (hits.length === 0) { teb.empty("search-results", "No results"); return; }
        el.innerHTML = hits.map(function (h) {
            return '<div class="card">'
                + '<span class="title">' + E(h.entity) + ' #' + h.entity_id + '</span>'
                + '<div class="desc">' + E(h.snippet) + '</div></div>';
        }).join("");
    });
};
/* gamification */
function loadStreak() {
    teb.api("GET", "/streak").then(function (r) {
        var el = document.getElementById("streak-info");
        if (r.error) { el.textContent = ""; return; }
        var s = r.streak || r;
        el.innerHTML = '<div class="grid">'
            + '<div class="stat"><div class="val">' + (s.current || 0)
            + '</div><div class="lbl">Current Streak</div></div>'
            + '<div class="stat"><div class="val">' + (s.longest || 0)
            + '</div><div class="lbl">Longest Streak</div></div></div>';
    });
}
function loadLeaderboard() {
    teb.api("GET", "/leaderboard").then(function (r) {
        var el = document.getElementById("leaderboard");
        if (r.error) { el.innerHTML = ""; return; }
        var rows = r.rows || r.entries || [];
        if (!Array.isArray(rows)) rows = [];
        if (rows.length === 0) { teb.empty("leaderboard", "No entries"); return; }
        el.innerHTML = '<table><tr><th>#</th><th>User</th><th>XP</th></tr>'
            + rows.map(function (e, i) {
                return '<tr><td>' + (i + 1) + '</td><td>' + E(e.user_id)
                    + '</td><td>' + (e.total_xp || e.amount || 0) + '</td></tr>';
            }).join("") + '</table>';
    });
}
window.creditXP = function () {
    var a = document.getElementById("xp-amount").value;
    var r = document.getElementById("xp-reason").value;
    if (!a) return;
    teb.api("POST", "/xp", { amount: parseInt(a, 10), reason: r })
        .then(function (res) {
            if (res.error) { teb.err(res.error); return; }
            teb.err(""); loadStreak(); loadLeaderboard();
        });
};
/* community */
window.load_community = function () { loadBlog(); };
function loadBlog() {
    teb.api("GET", "/blog/1").then(function (r) {
        var el = document.getElementById("blog-list");
        if (r.error) { teb.empty("blog-list", r.error); return; }
        var p = r.post || r;
        if (!p || !p.id) { teb.empty("blog-list", "No posts"); return; }
        el.innerHTML = '<div class="card">'
            + '<span class="title">' + E(p.title) + '</span>'
            + '<div class="desc">' + E(p.body) + '</div>'
            + '<div class="meta">By ' + E(p.user_id) + '</div></div>';
    });
}
window.postBlog = function () {
    var t = document.getElementById("blog-title").value;
    var b = document.getElementById("blog-body").value;
    if (!t || !b) return;
    teb.api("POST", "/blog", { title: t, body: b }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        document.getElementById("blog-title").value = "";
        document.getElementById("blog-body").value = "";
        teb.err(""); loadBlog();
    });
};
window.voteFeat = function () {
    var f = document.getElementById("vote-feature").value;
    if (!f) return;
    teb.api("POST", "/votes", { feature: f }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err("Vote recorded");
    });
};
/* workflows with 2s polling until complete */
window.load_workflows = function () {};
window.createRun = function () {
    var gid = document.getElementById("run-goal").value;
    if (!gid) return;
    teb.api("POST", "/runs", { goal_id: parseInt(gid, 10) }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err("");
        var rid = r.run_id || (r.run ? r.run.id : r.id);
        window.pollRun(rid, 0);
    });
};
window.pollRun = function (id, attempts) {
    teb.api("GET", "/runs/" + id).then(function (r) {
        var el = document.getElementById("run-status");
        if (r.error) { teb.empty("run-status", r.error); return; }
        var run = r.run || r;
        var steps = run.steps || [];
        var stepsHtml = steps.map(function (s) {
            return '<div class="meta">Step ' + s.id + ': '
                + E(s.agent) + ' — ' + E(s.status) + '</div>';
        }).join("");
        el.innerHTML = '<div class="card">'
            + '<span class="title">Run #' + run.id + '</span>'
            + ' <span class="status status-' + E(run.status || "running")
            + '">' + E(run.status || "running") + '</span>'
            + '<div class="meta">Goal ' + run.goal_id + '</div>'
            + stepsHtml + '</div>';
        /* Poll every 2s while running */
        if (run.status === "running" && attempts < 60) {
            setTimeout(function () { window.pollRun(id, attempts + 1); }, 2000);
        }
    });
};
window.checkRun = function (id) { window.pollRun(id, 0); };
/* outcomes & learnings */
window.load_measure = function () {};
window.storeOutcome = function () {
    var tid = document.getElementById("outcome-task").value;
    var res = document.getElementById("outcome-result").value;
    if (!tid || !res) return;
    teb.api("POST", "/outcomes", {
        task_id: parseInt(tid, 10), result: res
    }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err("Outcome stored");
    });
};
window.storeLearning = function () {
    var gid = document.getElementById("learn-goal").value;
    var ins = document.getElementById("learn-insight").value;
    if (!gid || !ins) return;
    teb.api("POST", "/learnings", {
        goal_id: parseInt(gid, 10), insight: ins
    }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        teb.err("Learning stored");
    });
};
})();
