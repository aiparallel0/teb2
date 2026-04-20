/* teb2 collab — workspaces, chat, integrations. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_collab = function () {
    loadWorkspaces();
    loadChat();
};
function loadChat() {
    var ws = document.getElementById("chat-ws").value || "1";
    teb.api("GET", "/chat/" + ws).then(function (r) {
        var el = document.getElementById("chat-messages");
        if (r.error) {
            el.innerHTML = '<div class="empty">' + E(teb.errmsg(r, "No messages")) + '</div>';
            return;
        }
        var rows = teb.list(r);
        if (rows.length === 0) {
            el.innerHTML = '<div class="empty">No messages yet</div>'; return;
        }
        el.innerHTML = rows.map(function (m) {
            return '<div class="chat-msg">'
                + '<span class="sender">' + E(m.sender_id || "msg #" + m.id) + '</span>'
                + '<div class="body">' + E(m.body) + '</div></div>';
        }).join("");
        el.scrollTop = el.scrollHeight;
    });
}
window.sendChat = function () {
    var ws = document.getElementById("chat-ws").value || "1";
    var b = document.getElementById("chat-input").value.trim();
    if (!b) return;
    teb.api("POST", "/chat", { ws_id: parseInt(ws, 10), body: b })
        .then(function (r) {
            if (r.error) { teb.err(teb.errmsg(r, "Send failed")); return; }
            document.getElementById("chat-input").value = "";
            teb.err(""); loadChat();
        });
};
window.createWorkspace = function () {
    var n = document.getElementById("ws-name").value.trim();
    if (!n) { teb.err("Workspace name required"); return; }
    teb.api("POST", "/workspaces", { name: n }).then(function (r) {
        if (r.error) { teb.err(teb.errmsg(r, "Could not create workspace")); return; }
        document.getElementById("ws-name").value = "";
        teb.err(""); teb.info("Workspace '" + n + "' created (ID " + r.id + ")");
        loadWorkspaces();
    });
};
function loadWorkspaces() {
    teb.api("GET", "/workspaces/1").then(function (r) {
        var el = document.getElementById("ws-list");
        if (r.error) {
            teb.empty("ws-list",
                r._status === 404 ? "No workspaces yet — create one above"
                                  : teb.errmsg(r, "Could not load workspaces"));
            return;
        }
        var w = r.ws || r;
        if (!w || !w.id) { teb.empty("ws-list", "No workspaces yet"); return; }
        el.innerHTML = '<div class="card">'
            + '<span class="title">' + E(w.name) + '</span>'
            + '<div class="meta">ID ' + w.id
            + (w.owner_id ? ' | Owner: ' + E(w.owner_id) : '')
            + '</div></div>';
    });
}
window.addCollab = function () {
    var ws = document.getElementById("collab-ws").value.trim();
    var u = document.getElementById("collab-user").value.trim();
    var rn = document.getElementById("collab-role").value.trim() || "member";
    if (!ws || !u) { teb.err("Workspace ID and user ID required"); return; }
    teb.api("POST", "/collabs", {
        ws_id: parseInt(ws, 10), user_id: u, role: rn
    }).then(function (res) {
        if (res.error) { teb.err(teb.errmsg(res, "Could not add collaborator")); return; }
        teb.err(""); teb.info("Collaborator added (ID " + res.id + ")");
    });
};
window.toggleInteg = function () {
    var n = document.getElementById("integ-name").value.trim();
    if (!n) { teb.err("Integration name required"); return; }
    teb.api("PUT", "/integrations", { name: n, enabled: 1 })
        .then(function (r) {
            if (r.error) {
                teb.err(r._status === 403
                    ? "Integrations are admin-only"
                    : teb.errmsg(r, "Could not toggle integration"));
                return;
            }
            teb.err(""); teb.info("Integration '" + n + "' enabled");
        });
};
/* SSE live chat */
window.startSSE = function () {
    var ws = document.getElementById("chat-ws").value || "1";
    var t = teb.token();
    if (!t) { teb.err("Login required for live chat"); return; }
    var src = new EventSource(teb.base + "/sse/chat/" + ws + "?token=" + encodeURIComponent(t));
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
    src.onerror = function () { src.close(); teb.info("Live chat connection closed"); };
    teb.info("Live chat active");
};
})();
