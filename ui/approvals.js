/* teb2 approvals — finance HITL confirm/deny. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;

window.load_approvals = function () {
    teb.api("GET", "/approvals?status=pending").then(render_list);
};

function render_list(r) {
    var el = document.getElementById("appr-list");
    if (!el) return;
    if (r.error) { teb.empty("appr-list", r.error); return; }
    var rows = r.rows || [];
    if (rows.length === 0) {
        teb.empty("appr-list", "No pending approvals");
        return;
    }
    el.innerHTML = rows.map(row_html).join("");
    rows.forEach(bind_buttons);
}

function row_html(a) {
    var amt = (a.amount_cents / 100).toFixed(2);
    var risk_color = a.risk === "HIGH" ? "#dc2626"
                   : a.risk === "MEDIUM" ? "#d97706" : "#16a34a";
    return '<div class="card" id="appr-' + a.id + '">'
         + '<span class="title">Approval #' + a.id + '</span>'
         + '<div class="meta">Amount: $' + E(amt)
         + ' | Risk: <span style="color:' + risk_color + '">'
         + E(a.risk || "?") + '</span>'
         + ' | Kind: ' + E(a.kind) + '</div>'
         + '<pre style="font-size:.85em;white-space:pre-wrap;'
         + 'background:#1a1a1a;padding:.5rem;border-radius:3px">'
         + E(a.payload || "") + '</pre>'
         + '<div style="margin-top:.5rem">'
         + '<button class="btn-primary" data-appr-approve="' + a.id + '">'
         + 'Approve</button> '
         + '<button data-appr-deny="' + a.id + '">Deny</button>'
         + '</div></div>';
}

function bind_buttons(a) {
    var ap = document.querySelector('[data-appr-approve="' + a.id + '"]');
    var dn = document.querySelector('[data-appr-deny="' + a.id + '"]');
    if (ap) ap.onclick = function () { act(a.id, "approve"); };
    if (dn) dn.onclick = function () { act(a.id, "deny"); };
}

function act(id, verb) {
    teb.api("POST", "/approvals/" + id + "/" + verb, {}).then(function (r) {
        if (r.error) {
            alert("Failed: " + r.error);
            return;
        }
        var row = document.getElementById("appr-" + id);
        if (row) row.remove();
    });
}
})();
