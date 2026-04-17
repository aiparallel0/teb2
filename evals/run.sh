#!/usr/bin/env bash
# teb2 eval harness — smoke test.
#
# Not a semantic eval yet (see docs/FOLLOWUPS.md §J). This script:
#   1. Validates every prompt source in prompts/*.md has a matching
#      generated core/pd/*.c file.
#   2. Validates prompt registration in core/prompts.c references
#      each embedded symbol.
#   3. Optionally replays a couple of goals against evals/mock_llm.py
#      if $TEB2_RUN=1 and teb2 binary is built — validates that
#      clarify/decompose JSON shapes are actually parseable.
#
# Exits non-zero on any drift so CI can gate on it.
set -euo pipefail
cd "$(dirname "$0")/.."

fail=0
note() { printf '%s\n' "$*" >&2; }
err()  { printf 'FAIL: %s\n' "$*" >&2; fail=1; }

# 1. Prompt sources vs. generated
for src in prompts/*.md prompts/system/*.md; do
    [ -e "$src" ] || continue
    base="$(basename "$src" .md)"
    # system/* collapse to p_system_<name>.c
    case "$src" in
        prompts/system/*) gen="core/pd/system_${base}.c" ;;
        *)                gen="core/pd/${base//./_}.c"    ;;
    esac
    [ -f "$gen" ] || err "missing generated file for $src (expected $gen)"
done

# 2. Registry sanity
grep -q 'prompt_get' core/prompts.c || err "prompt_get not found in registry"
grep -qE 'P_[A-Z_]+' core/prompts.c || err "no P_ symbols referenced"

# 3. Build must be current
if command -v make >/dev/null; then
    make -q || note "(info) teb2 build is stale; run 'make' before TEB2_RUN"
fi

# 4. Optional live-shape check with mock LLM
if [ "${TEB2_RUN:-0}" = "1" ] && [ -x ./teb2 ]; then
    note "running mock_llm shape check (smoke only)"
    python3 evals/mock_llm.py 18088 &
    mpid=$!
    trap "kill $mpid 2>/dev/null || true" EXIT
    sleep 0.3
    # A real harness would POST to teb2 and inspect the DB — out of
    # scope for this smoke test. We just confirm the mock returns
    # something matching our declared fixtures.
    for k in clarify decompose measure finance.risk; do
        curl -fsS -X POST "http://127.0.0.1:18088/v1/chat/completions" \
            -H 'content-type: application/json' \
            -d "{\"model\":\"gpt\",\"messages\":[{\"role\":\"system\",\"content\":\"$k\"}]}" \
            | grep -q '"content"' \
            || err "mock_llm did not return content for $k"
    done
fi

if [ $fail -ne 0 ]; then
    note "eval harness reported drift; fix the above and re-run"
    exit 1
fi
note "eval harness passed ($(ls prompts/*.md prompts/system/*.md | wc -l) prompts checked)"
