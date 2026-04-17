# Follow-ups

This document tracks audit gaps that are **not** addressed by the current
PR. They are deferred because each requires a larger architectural
decision or a dependency the current PR cannot justify.

Phases below use the letters from the audit/plan so reviewers can trace
back to the original gap. **Status table regenerated from reality** —
previous versions of this file marked items as "closed" while the
vestige code was still wired up. We now prefer honest "partial" labels.

## Phase A — Stop the lying *(this PR)*

Done in this PR:

- **A1.** `agents/decompose.c` now extracts `depends_on`,
  `effort_minutes`, `est_cost_cents`, `requires_hitl`, and
  `success_criteria` from the model reply and persists them into a new
  `task_plan` table (`db/task_plan.c`). The scheduler, retry logic,
  and HITL gate can finally see what the prompt already asks the model
  to emit.
- **A2.** `agents/measure.c` now extracts `next_action` and writes it
  to `task_plan` alongside `score_0_100` and an attempt counter. A
  run supervisor (Phase B) can read this to re-enqueue on "retry" /
  "escalate".
- **A3.** `agents/learn.c` now extracts the scalar `insight` string
  out of the JSON envelope before persisting to `learnings.insight`.
  Previously the entire JSON envelope was stored, which poisoned every
  subsequent `list_learnings` context injection.
- **A4.** Added `MSG_RESEARCH`; `api/exec.c` and `api/workflow.c`
  route research tasks through it. `MSG_EXEC_REQ → research_handle`
  remains as a temporary legacy shim until all callers migrate.
- **A7.** `evals/mock_llm.py` learn fixture was `confidence: 0.7`
  (number) while the prompt declares `"low"|"medium"|"high"`
  (string). Fixed to `"medium"`. Schema drift now caught by G1.
- **A8.** README file-inventory reconciled with `find` output.

Still open in Phase A:

- **A5.** `sanitize_untrusted` output buffer and `HttpReq.body` still
  truncate silently. Needs a size bump + explicit `413` path.

## Phase B — Run supervision, cancellation, token budget *(deferred)*

`api/workflow.c::handle_run_create` still forks a child with no wait,
timeout, or cancel endpoint. Real supervision needs:

- A `runs` row state machine (queued / running / cancelled / failed /
  timed_out) with `token_spend_cents` aggregated from `LlmReply`.
- SIGCHLD handler reaping so zombies don't accumulate under macOS.
- `POST /runs/<id>/cancel` endpoint sending SIGTERM.
- Per-run token budget enforced in `core/llm.c`.

## Phase C — Multi-provider, model choice, failover *(deferred)*

`core/llm.c` still hardcodes OpenAI, one model, one `max_tokens=1024`,
no retry, no backoff. A provider abstraction plus per-agent model
overrides ("finance.risk uses the stronger reasoning model;
outreach.nudge uses the cheap one") needs a config story
(`cfg->providers[].base_url`, `.key`) and jittered backoff for 429/5xx.

## Phase D — Grounded research *(deferred)*

`agents/research.c` still prompts the LLM without a retrieval step.
Shipping this safely means:

1. Pick a search API (Brave / SerpAPI / Tavily) and its credentials
   story.
2. Add a `search_snippet` table for citation persistence.
3. Extend `prompts/research.md` to require citations that resolve to
   rows in `search_snippet`.
4. Add a validator in `agents/research.c` that rejects uncited facts
   and re-runs once with a widened query.

## Phase E — Browser driver end-to-end *(deferred)*

`exec/browser_spawn.c` spawns a Node subprocess and
`exec/browser_worker.js` speaks a tab-separated protocol, but no
`MSG_BROWSE` tag is defined, no `agents/browser.c` exists, and the
pipe FDs are never threaded into `cred->login` where
`exec/browser.c::browser_send` looks for them. Shipping this safely
needs the agent handler, the FD plumbing, a sandbox policy, and a
`--requires_confirmation` HITL gate before any write-action is
executed.

## Phase F — Types split & `api/` trim *(deferred)*

`core/types.h` is at 165/166. `core/types_ext.h` is at 159/166. The
plan calls for splitting into `types_common.h` + `types_goals.h` +
`types_tasks.h` + `types_llm.h`. Mechanical but wide. Also: 32 files
in `api/` vs. the retired "23-file minimal kernel" story — candidate
for a dedicated clean-up PR.

## Phase G — Golden-set + red-team LLM evals *(partial in this PR)*

Partial in this PR: `evals/golden/*.jsonl` + `evals/redteam/*.jsonl`
sample fixtures plus a shape validator wired into `evals/run.sh`. A
real eval harness still needs:

- A JSON-Schema file per prompt, checked in and validated round-trip
  against the prompt text.
- A runner that calls the real LLM, scores outputs against a rubric,
  and writes a per-agent JSON report with pass@1.
- CI gate on score regression, not just build success.
- OWASP LLM top-10 red-team corpus replayed against every prompt on
  every PR.

## Phase H — Prompt library toward n8n/Notion bar *(advanced in this PR)*

Previously: pushed the library from 43 → 53 → 77 prompts and introduced
the `## Tool manifest` section on every prompt.

This PR adds **22 more prompts** (77 → 99) targeting the concrete
workflow domains teams currently reach for n8n / Zapier / Notion
templates to solve:

- Customer support — `support.reply`, `support.macro_suggest`, `support.faq_generate`
- Sales — `sales.discovery_notes`, `sales.proposal_draft`
- Marketing — `marketing.landing_copy`, `marketing.newsletter`
- Product — `product.okr_draft`, `product.project_brief`
- HR — `hr.performance_review`, `hr.one_on_one`, `hr.offboarding_checklist`
- Ops / SRE — `ops.deploy_plan`, `ops.rollback_plan`, `ops.dependency_advisory`
- Data / analytics — `data.metric_definition`, `data.sql_explain`
- Engineering — `code.migration_plan`
- Finance — `finance.invoice_review`, `finance.budget_variance`
- Personal productivity — `task.daily_standup`, `task.inbox_triage`

Each new prompt follows the established template (Role / Input /
JSON Output / Rules / Example / Anti-example / Refusal / Injection
hardening / Tool manifest) and stays within the 166-line cap on its
generated C file. Safety-sensitive additions (performance review,
offboarding, deploy/rollback, advisory triage, invoice review,
support reply, inbox triage) ship red-team fixtures under
`evals/redteam/new_prompts2.jsonl` so CI drift-locks the prompt
names and expectation shapes. See `docs/PROMPT_CATALOG.md` for a
workflow-oriented index.

Still needed:

- A real eval runner (not just a drift guard): replay every
  `evals/golden/*.jsonl` against a model, score vs `expect`,
  aggregate pass@1, gate CI on regression. The current
  `evals/run.sh` still only shape-checks fixtures.
- Expand every prompt to 150–250 lines with 5+ worked examples, a
  failure-mode catalogue, and a JSON-Schema block round-trip checked
  against the prompt text.
- Prompt versioning at runtime (content hash stored on every agent
  call in a new `prompt_runs` table).
- Planner/critic pairs invoked automatically after decompose/measure.
- Per-tenant override plumbing (`prompt_override(tenant_id, name)`).
- Wire `safety.injection_detect` in-line on all untrusted user text
  before it reaches downstream agent prompts.

## Phase I — Production hygiene *(partial in this PR)*

Partial: `PRAGMA busy_timeout=5000` so worker forks don't bounce off
`SQLITE_BUSY`. Still needed: structured logs, per-agent latency +
token histograms on `/metrics`, `data.redact` auto-invoked before
persisting outcomes/learnings, secret rotation via `exec/vault.c`.

## Phase J — UX catch-up *(deferred)*

Prompts editor edit+diff+eval run, runs timeline with cancel,
approvals tab, audit-log viewer. Blocked on Phase B (runs) and
Phase H overrides.

## ~~Phase X — `exec` dead-letter bug~~ *(partially addressed, this PR finishes)*

Previous PR claimed this closed but left `MSG_EXEC_REQ → research_handle`
wired in `agents/coord.c` as the only research path. This PR moves
research to an explicit `MSG_RESEARCH` tag; the legacy route is
retained as a temporary shim and will be removed once all callers
migrate.

