# Verification

## Human-only prerequisites (done once by the repo owner)

# 1. Provision an Ubuntu 24.04 host and point the portearchive.com A record at it.
# 2. In GitHub repo settings, create four Actions secrets:
#    DEPLOY_HOST, DEPLOY_USER, DEPLOY_SSH_KEY, DOMAIN=portearchive.com
# 3. SSH to the host once and run:
sudo DOMAIN=portearchive.com bash /opt/teb2/server-setup.sh

## Acceptance checks (run from any laptop with curl + a browser)

# HTTPS root returns 200 and serves the ui/ frontend
curl -sS -o /dev/null -w "%{http_code}\n" https://portearchive.com/

# Health endpoint returns JSON {"status":"ok",...}
curl -sS https://portearchive.com/healthz

# Browser: open https://portearchive.com/ — register, log in, submit a goal,
# reload, confirm the goal persists and transitions state via the exec/ layer.

# Re-deploy on every push to main is automatic via .github/workflows/deploy.yml.
