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
| Router (lib)       | `agents/coord.c`      | `router.md`              | `{agent, confidence}` — *optional*     |
| Research           | `agents/research.c`   | `research.md`            | `{summary, key_facts, confidence}`     |
| Browser plan (lib) | (driver pending)      | `browse.md`              | `{plan:[…], stop_conditions:[…]}`      |
| Measure            | `agents/measure.c`    | `measure.md`             | `{score_0_100, rubric, next_action}`   |
| Learn              | `agents/learn.c`      | `learn.md`               | `{insight, tags, generalizes_to}`      |
| Outreach (nudge)   | `agents/outreach.c`   | `outreach.nudge.md`      | `{message, tone, references_learnings}`|
| Outreach (notify)  | `agents/outreach.c`   | `outreach.notify.md`     | SMTP subject/body/urgency              |
| Finance (risk)     | `agents/finance.c`    | `finance.risk.md`        | `{risk, signals, recommend}`           |
| Plugin (webhook)   | `agents/plugin.c`     | `plugin.webhook.md`      | `{headers, body_json}`                 |

Every LLM call also gets `system/persona.md` + `system/guardrails.md`
prepended automatically by `core/llm.c`.

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

See `docs/FOLLOWUPS.md` for phases not yet shipped (router promotion,
grounded research, multi-provider, run supervision, types-split).
