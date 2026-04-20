/* teb2 prompts viewer — read-only registry (admin). ≤166 lines */
(function () {
"use strict";
var E = teb.esc;

window.load_prompts = function () {
    teb.api("GET", "/prompts").then(render_list);
};

function render_list(r) {
    var el = document.getElementById("prompt-list");
    if (!el) return;
    if (r.error) { teb.empty("prompt-list", r._status === 403 ? "Prompt registry is admin-only" : teb.errmsg(r, "Could not load prompts")); return; }
    var names = r.names || [];
    if (names.length === 0) {
        teb.empty("prompt-list", "No prompts registered");
        return;
    }
    el.innerHTML = '<div class="card">'
        + '<span class="title">Prompt registry (' + names.length + ')</span>'
        + '<ul id="prompt-names" style="list-style:none;padding:0">'
        + names.map(function (n) {
            return '<li style="margin:.25rem 0">'
                 + '<a href="#" data-prompt="' + E(n) + '" '
                 + 'style="color:#60a5fa;text-decoration:none">'
                 + E(n) + '</a></li>';
          }).join("")
        + '</ul></div>'
        + '<div id="prompt-body"></div>';
    document.querySelectorAll("[data-prompt]").forEach(function (a) {
        a.onclick = function (ev) {
            ev.preventDefault();
            load_body(a.getAttribute("data-prompt"));
        };
    });
}

function load_body(name) {
    /* prompts return text/plain; teb.api parses JSON, so use fetch directly. */
    fetch(teb.base + "/prompts/" + encodeURIComponent(name), {
        credentials: "same-origin",
        headers: teb.auth_headers ? teb.auth_headers() : {}
    }).then(function (res) {
        return res.text().then(function (t) {
            return { status: res.status, body: t };
        });
    }).then(function (r) {
        var el = document.getElementById("prompt-body");
        if (!el) return;
        if (r.status !== 200) {
            el.innerHTML = '<div class="card">Error ' + r.status + '</div>';
            return;
        }
        el.innerHTML = '<div class="card">'
            + '<span class="title">' + E(name) + '</span>'
            + '<pre style="white-space:pre-wrap;font-size:.85em;'
            + 'background:#1a1a1a;padding:.75rem;border-radius:4px;'
            + 'max-height:60vh;overflow:auto">'
            + E(r.body) + '</pre></div>';
    });
}
})();
