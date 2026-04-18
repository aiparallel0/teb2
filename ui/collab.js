/* teb2 collab — workspaces, chat, integrations. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_collab = function () {
    loadChat();
};
function loadChat() {
    var ws = document.getElementById("chat-ws").value || "1";
    teb.api("GET", "/chat/" + ws).then(function (r) {
        var el = document.getElementById("chat-messages");
        if (r.error) { el.innerHTML = '<div class="empty">No messages</div>'; return; }
        var rows = r.rows || r.messages || [];
        if (!Array.isArray(rows)) rows = [];
        if (rows.length === 0) {
            el.innerHTML = '<div class="empty">No messages yet</div>'; return;
        }
        el.innerHTML = rows.map(function (m) {
            return '<div class="chat-msg">'
                + '<span class="sender">' + E(m.sender_id) + '</span>'
                + '<div class="body">' + E(m.body) + '</div></div>';
        }).join("");
        el.scrollTop = el.scrollHeight;
    });
}
window.sendChat = function () {
    var ws = document.getElementById("chat-ws").value || "1";
    var b = document.getElementById("chat-input").value;
    if (!b) return;
    teb.api("POST", "/chat", { ws_id: parseInt(ws, 10), body: b })
        .then(function (r) {
            if (r.error) { teb.err(r.error); return; }
            document.getElementById("chat-input").value = "";
            teb.err(""); loadChat();
        });
};
window.createWorkspace = function () {
    var n = document.getElementById("ws-name").value;
    if (!n) return;
    teb.api("POST", "/workspaces", { name: n }).then(function (r) {
        if (r.error) { teb.err(r.error); return; }
        document.getElementById("ws-name").value = "";
        teb.err(""); loadWorkspaces();
    });
};
function loadWorkspaces() {
    teb.api("GET", "/workspaces/1").then(function (r) {
        var el = document.getElementById("ws-list");
        if (r.error) { teb.empty("ws-list", r.error); return; }
        var w = r.ws || r;
        if (!w || !w.id) { teb.empty("ws-list", "No workspaces"); return; }
        el.innerHTML = '<div class="card">'
            + '<span class="title">' + E(w.name) + '</span>'
            + '<div class="meta">ID ' + w.id + ' | Owner: ' + E(w.owner_id)
            + '</div></div>';
    });
}
window.addCollab = function () {
    var ws = document.getElementById("collab-ws").value;
    var u = document.getElementById("collab-user").value;
    var r = document.getElementById("collab-role").value || "member";
    if (!ws || !u) return;
    teb.api("POST", "/collabs", {
        ws_id: parseInt(ws, 10), user_id: u, role_name: r
    }).then(function (res) {
        if (res.error) { teb.err(res.error); return; }
        teb.err("Collaborator added");
    });
};
window.toggleInteg = function () {
    var n = document.getElementById("integ-name").value;
    if (!n) return;
    teb.api("PUT", "/integrations", { name: n, enabled: 1 })
        .then(function (r) {
            if (r.error) { teb.err(r.error); return; }
            teb.err("Integration enabled");
        });
};
/* SSE live chat */
window.startSSE = function () {
    var ws = document.getElementById("chat-ws").value || "1";
    var t = teb.token();
    if (!t) { teb.err("Login required for live chat"); return; }
    var src = new EventSource("/sse/chat/" + ws + "?token=" + t);
    src.onmessage = function (e) {
        try {
            var m = JSON.parse(e.data);
            var el = document.getElementById("chat-messages");
            el.innerHTML += '<div class="chat-msg">'
                + '<span class="sender">live</span>'
                + '<div class="body">' + E(m.body) + '</div></div>';
            el.scrollTop = el.scrollHeight;
        } catch (ex) { /* ignore parse errors */ }
    };
    src.onerror = function () { src.close(); };
};
})();
