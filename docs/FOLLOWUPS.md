# Follow-ups

This document tracks audit gaps that are **not** addressed by the MVP
shipped alongside it. They are deferred because each requires a larger
architectural decision or a dependency the current PR cannot justify.

Phases below use the letters from the audit/plan so reviewers can trace
back to the original gap.

## Phase D — Router promotion to a first-class agent

Today `decompose` asks the LLM to choose an `agent` per task directly.
The plan calls for a separate `router.md` classifier invoked after
decomposition, so every task gets a second opinion and a confidence
score. The prompt is already committed in `prompts/router.md`; the
runtime wiring (a per-task `router_handle` call from `coord.c`) is
deferred because it changes the run-loop contract and deserves its own
review.

## Phase E — Grounded research

`agents/research.c` still prompts the LLM without a retrieval step. The
infrastructure exists (`exec/http.c`, `exec/tls.c`) but no search
provider is wired in and no snippet cache is defined. Shipping this
safely means:

1. Pick a search API (Brave / SerpAPI / Tavily) and its credentials
   story.
2. Add a `search_snippet` table for citation persistence.
3. Extend `prompts/research.md` to require citations that resolve to
   rows in `search_snippet`.
4. Add a validator in `agents/research.c` that rejects uncited facts.

## Phase G — Multi-provider, model choice, failover

`core/llm.c` still hardcodes OpenAI. A provider abstraction (Anthropic,
Ollama, Azure) plus per-agent model overrides (“finance.risk uses the
stronger reasoning model; outreach.nudge uses the cheap one”) is high
value but needs a config story (`cfg->providers[].base_url`,
`cfg->providers[].key`) and retry policy that we did not want to bolt
on in this PR.

## Phase I — Run supervision, cancellation, zombie reaping

`api/workflow.c::handle_run_create` still forks a child with no wait,
timeout, or cancel endpoint. Real supervision needs:

- A `runs` row state machine (queued / running / cancelled / failed).
- SIGCHLD handler reaping so zombies don't accumulate.
- `POST /runs/<id>/cancel` endpoint sending SIGTERM.
- Per-run token budget so a rogue loop cannot burn unlimited tokens.

## Phase L.2 — Splitting `core/types.h`

`core/types.h` sits at 163/166 lines. Any new domain struct breaks the
cap. The plan calls for splitting into `types_goals.h` + `types_tasks.h`
+ keeping the common forward declarations in `types.h`. This is a
mechanical but wide change and we kept it out of this PR so the diff
stays reviewable.

## Phase L.3 — Trimming `api/`

33 files in `api/` is out of line with the 23-file story in the README.
A clean-up pass (folding CRUD endpoints per domain, deleting the
admin-only community / gamification experiments that no workflow
depends on) is a candidate for a dedicated PR.

## Phase K-full — Prompt edit + run diff in the UI

The current PR ships a **read-only** prompt viewer. Editable prompts
with per-tenant overrides, diffing against baseline, and an A/B runner
are a larger feature — they need:

- A `prompt_override` table keyed on `(tenant_id, prompt_name, version)`.
- Version pinning per agent call.
- An evaluation UI (“this override lifted decompose quality from 62 to
  74 on the golden set”) which requires Phase J-full first.

## Phase J-full — Golden-set LLM evals

`evals/run.sh` currently runs only shape checks against
`evals/mock_llm.py`. A real eval harness needs:

- `evals/goals.jsonl` extended with expected envelopes.
- A runner that calls the real LLM, scores outputs against a rubric,
  and writes a per-agent JSON report.
- CI integration gated on score regression, not just build success.
