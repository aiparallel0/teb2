# Agents

teb2 runs every task through a small C function that assembles a named
prompt, calls the LLM, validates the JSON envelope, and persists the
result. There is no framework magic. Each agent is one file, ≤166 lines,
with a single entry point declared in `agents/channel.h`.

## The loop

    Goal → Clarify → Decompose → Execute → Measure → Learn
              ↑                                         │
              └─────────────── learnings ───────────────┘

`Learn` writes durable insights that `Clarify`, `Decompose`, and the
`outreach.nudge` agent read back on their next run. That loop closes
the feedback cycle the audit flagged as Gap 4.

## Agent table

| Agent              | Source                | Prompt (`prompts/…`)     | Emits                                  |
| ------------------ | --------------------- | ------------------------ | -------------------------------------- |
| Clarify            | `agents/clarify.c`    | `clarify.md`             | `{status, questions, readiness_score}` |
| Decompose          | `agents/decompose.c`  | `decompose.md`           | `{tasks:[{title,agent,depends_on,…}]}` |
| Router             | `agents/router.c`     | `router.md`              | `{agent, confidence, rationale}`       |
| Research           | `agents/research.c`   | `research.md`            | `{summary, key_facts, confidence}`     |
| Browser plan (lib) | (driver pending)      | `browse.md`              | `{plan:[…], stop_conditions:[…]}`      |
| Measure            | `agents/measure.c`    | `measure.md`             | `{score_0_100, rubric, next_action}`   |
| Learn              | `agents/learn.c`      | `learn.md`               | `{insight, tags, generalizes_to}`      |
| Exec (dispatcher)  | `agents/exec.c`       | `exec.*.md` (12 sub)     | Schema of chosen `exec.*` prompt       |
| Outreach           | `agents/outreach.c`   | `outreach.*.md` (6 sub)  | Schema of chosen `outreach.*` prompt   |
| Finance            | `agents/finance.c`    | `finance.*.md` (4 sub)   | Schema of chosen `finance.*` prompt    |
| Plugin             | `agents/plugin.c`     | `plugin.*.md` (3 sub)    | Schema of chosen `plugin.*` prompt     |

Every LLM call also gets `system/persona.md` + `system/guardrails.md`
prepended automatically by `core/llm.c`.

## Prompt catalog (43 total)

Prompts are grouped by namespace. Each file follows the 8-section
structure mandated by `docs/PROMPTS.md`.

- **System** (2): `system/persona`, `system/guardrails`.
- **Loop** (7): `clarify`, `decompose`, `router`, `research`,
  `browse`, `measure`, `learn`.
- **Exec** (12): `exec.code`, `exec.code_review`, `exec.refactor`,
  `exec.write`, `exec.summarize`, `exec.extract`, `exec.classify`,
  `exec.sql`, `exec.translate`, `exec.rewrite`, `exec.sentiment`,
  `exec.plan`.
- **Outreach** (6): `outreach.nudge`, `outreach.notify`,
  `outreach.reply`, `outreach.cold`, `outreach.followup`,
  `outreach.apology`.
- **Finance** (4): `finance.risk`, `finance.forecast`,
  `finance.categorize`, `finance.receipt`.
- **Plugin** (3): `plugin.webhook`, `plugin.oauth_choose`,
  `plugin.error_repair`.
- **Data** (3): `data.redact`, `data.moderate`, `data.json_repair`.
- **Meeting** (3): `meeting.agenda`, `meeting.notes`, `meeting.retro`.
- **Doc** (2): `doc.qa`, `doc.outline`.
- **Ops** (1): `triage.ticket`.

## How `exec.*` is picked

When Decompose assigns `agent="exec"` to a task, `api/exec.c` emits
`MSG_EXEC_RUN`, which `agents/coord.c` routes to
`agents/exec.c::exec_handle`. That handler picks a specific
`exec.*` prompt from a keyword table over the task description
(`code_review`, `refactor`, `sql`, `translate`, `summarize`,
`extract`, `classify`, `sentiment`, `rewrite`, `write`, `plan`,
`code`). If no keyword matches, the default is `exec.plan` — the
handler refuses to guess and instead produces a plan for a
subsequent focused call.

## Invariants

1. **No raw user text in the system prompt.** All untrusted strings pass
   through `sanitize_untrusted` (`core/sanitize.c`) and are delivered in
   the `user` role wrapped in `<untrusted_input>`.
2. **Structured output.** Every agent that can benefit from JSON sets
   `LlmReq.want_json = 1`, which makes `core/llm.c` request
   `response_format={type:"json_object"}`.
3. **Named prompts only.** `LlmReq.prompt_name` is the only way to
   specify behavior; there is no free-form `system` field any more.
4. **≤166 lines per file.** CI enforces this. Agents that need more
   helpers must spin them into sibling files (example: each prompt body
   lives in `core/pd/<name>.c`).

## Adding an agent

1. Write `prompts/<name>.md` with the full catalog (role, objective,
   context, schema, worked examples, refusal, injection-hardening).
2. Run `make prompts` to regenerate `core/pd/<name>.c`.
3. Register `<name>` in the `TABLE` at the top of `core/prompts.c`.
4. Add `<your_name>_handle(AgentMsg)` in `agents/<name>.c` that sets
   `LlmReq.prompt_name` and, if relevant, injects context via
   `list_learnings` / `list_mem` from `db/db.h`.
5. Declare the handler in `agents/channel.h`.
6. Extend `evals/mock_llm.py` with a fixture so the smoke harness
   keeps covering the new agent.

## Deferred work

See `docs/FOLLOWUPS.md` for phases not yet shipped. The gaps this PR
closed — `exec` dead-letter bug, anaemic prompt catalog (13→43),
router callability — are struck through there. Still open: grounded
research (§E), multi-provider (§G), run supervision (§I), types-split
(§L.2), golden-set evals (§J), editable per-tenant prompts (§K),
browser driver.
