/* teb2 enterprise — orgs, SSO, IP allowlist. ≤166 lines */
(function () {
"use strict";
var E = teb.esc;
window.load_enterprise = function () {
    loadOrg();
};
function loadOrg() {
    teb.api("GET", "/orgs/1").then(function (r) {
        if (r.error) {
            teb.empty("org-detail", r._status === 404
                ? "No organization yet — create one above"
                : teb.errmsg(r, "Could not load org"));
            return;
        }
        var o = r.org || r;
        if (!o || !o.id) { teb.empty("org-detail", "No organization"); return; }
        document.getElementById("org-detail").innerHTML = '<div class="card">'
            + '<span class="title">' + E(o.name) + '</span>'
            + '<div class="meta">Domain: ' + E(o.domain) + ' | ID: ' + o.id + '</div>'
            + '</div>';
    });
}
window.createOrg = function () {
    var n = document.getElementById("org-name").value.trim();
    var d = document.getElementById("org-domain").value.trim();
    if (!n || !d) { teb.err("Name and domain required"); return; }
    teb.api("POST", "/orgs", { name: n, domain: d }).then(function (r) {
        if (r.error) {
            teb.err(r._status === 403
                ? "Only admins can create organizations"
                : teb.errmsg(r, "Could not create org"));
            return;
        }
        document.getElementById("org-name").value = "";
        document.getElementById("org-domain").value = "";
        teb.err(""); teb.info("Org '" + n + "' created (ID " + r.id + ")");
        loadOrg();
    });
};
window.validateSSO = function () {
    var org = document.getElementById("sso-org").value.trim();
    var prov = document.getElementById("sso-provider").value.trim();
    if (!org || !prov) { teb.err("Org ID and provider required"); return; }
    teb.api("POST", "/sso/validate", {
        org_id: parseInt(org, 10), provider: prov
    }).then(function (r) {
        var el = document.getElementById("sso-result");
        if (r.error) {
            el.innerHTML = '<div class="card"><span class="status status-failed">'
                + E(teb.errmsg(r, "SSO validation failed")) + '</span></div>';
            return;
        }
        el.innerHTML = '<div class="card"><span class="status status-active">'
            + 'SSO valid</span>'
            + '<div class="meta">Provider: ' + E((r.sso || r).provider || prov) + '</div>'
            + '</div>';
    });
};
window.checkIP = function () {
    var org = document.getElementById("ip-org").value.trim();
    var cidr = document.getElementById("ip-cidr").value.trim();
    if (!org || !cidr) { teb.err("Org ID and CIDR required"); return; }
    teb.api("POST", "/ip/check", {
        org_id: parseInt(org, 10), cidr: cidr
    }).then(function (r) {
        var el = document.getElementById("ip-result");
        if (r.error) {
            el.innerHTML = '<div class="card"><span class="status status-failed">'
                + E(teb.errmsg(r, "IP check failed")) + '</span></div>';
            return;
        }
        var allowed = (r.allowed === true) || (r.allowed === undefined);
        el.innerHTML = '<div class="card"><span class="status status-'
            + (allowed ? 'active' : 'failed') + '">'
            + (allowed ? 'IP allowed' : 'IP denied') + '</span>'
            + '<div class="meta">CIDR: ' + E(cidr) + '</div>'
            + '</div>';
    });
};
})();
