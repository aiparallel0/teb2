/* teb2 dashboard — search, gamification, community, workflows, measure. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_dash = function () {
    loadStreak(); loadLeaderboard();
};
/* search */
window.doSearch = function () {
    var q = document.getElementById("search-q").value.trim();
    if (!q) { teb.err("Enter a search query"); return; }
    teb.api("GET", "/search?q=" + encodeURIComponent(q)).then(function (r) {
        if (r.error) { teb.empty("search-results", teb.errmsg(r, "Search failed")); return; }
        var hits = teb.list(r);
        if (hits.length === 0) { teb.empty("search-results", "No results for \"" + q + "\""); return; }
        document.getElementById("search-results").innerHTML = hits.map(function (h) {
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
        if (r.error) { teb.empty("leaderboard", teb.errmsg(r, "Could not load leaderboard")); return; }
        var rows = teb.list(r);
        if (rows.length === 0) { teb.empty("leaderboard", "No XP awarded yet"); return; }
        document.getElementById("leaderboard").innerHTML =
            '<table><tr><th>#</th><th>User</th><th>XP</th></tr>'
            + rows.map(function (e, i) {
                return '<tr><td>' + (i + 1) + '</td><td>' + E(e.user_id)
                    + '</td><td>' + (e.total || e.total_xp || e.amount || 0) + '</td></tr>';
            }).join("") + '</table>';
    });
}
window.creditXP = function () {
    var a = document.getElementById("xp-amount").value.trim(),
        r = document.getElementById("xp-reason").value.trim();
    if (!a) { teb.err("XP amount required"); return; }
    teb.api("POST", "/xp", { amount: parseInt(a, 10), reason: r })
        .then(function (res) {
            if (res.error) { teb.err(teb.errmsg(res, "Could not award XP")); return; }
            teb.err(""); teb.info("Awarded " + a + " XP");
            document.getElementById("xp-amount").value = "";
            document.getElementById("xp-reason").value = "";
            loadStreak(); loadLeaderboard();
        });
};
/* community */
window.load_community = function () { loadBlog(); };
function loadBlog() {
    teb.api("GET", "/blog/1").then(function (r) {
        if (r.error) {
            teb.empty("blog-list", r._status === 404
                ? "No posts yet — write one above"
                : teb.errmsg(r, "Could not load blog"));
            return;
        }
        var p = r.post || r;
        if (!p || !p.id) { teb.empty("blog-list", "No posts yet"); return; }
        document.getElementById("blog-list").innerHTML = '<div class="card">'
            + '<span class="title">' + E(p.title) + '</span>'
            + '<div class="desc">' + E(p.body) + '</div>'
            + (p.user_id ? '<div class="meta">By ' + E(p.user_id) + '</div>' : '')
            + '</div>';
    });
}
window.postBlog = function () {
    var t = document.getElementById("blog-title").value.trim(),
        b = document.getElementById("blog-body").value.trim();
    if (!t || !b) { teb.err("Title and body required"); return; }
    teb.api("POST", "/blog", { title: t, body: b }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Post failed")); return; }
        document.getElementById("blog-title").value = "";
        document.getElementById("blog-body").value = "";
        teb.err(""); teb.info("Post published"); loadBlog();
    });
};
window.voteFeat = function () {
    var f = document.getElementById("vote-feature").value.trim();
    if (!f) { teb.err("Feature name required"); return; }
    teb.api("POST", "/votes", { feature: f }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Vote failed")); return; }
        teb.err("");
        teb.info("Vote recorded — " + E(f) + " has " + (r.count || 1) + " vote(s)");
    });
};
})();
