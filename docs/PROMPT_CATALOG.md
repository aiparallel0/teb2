# Prompt Catalog — by Workflow

This is the workflow-oriented index of every prompt in teb2. Whereas
[`PROMPTS.md`](PROMPTS.md) groups prompts by the C namespace they land
in, this file groups them by the job a user is trying to get done —
closer to how n8n lists nodes or Notion lists templates. Every prompt
below exists as a file under `prompts/` and is compiled into the
binary via `make prompts`.

All prompts follow the same contract: JSON-only output, explicit
refusal block, injection-hardening clause, and a tool manifest that
names the downstream system each field drives.

## The core loop (what teb2 does with any goal)

| Prompt              | What it decides                                                           |
| ------------------- | ------------------------------------------------------------------------- |
| `system/persona`    | The persona every agent inherits. Prepended automatically.                |
| `system/guardrails` | The guardrails every agent inherits. Prepended automatically.             |
| `clarify`           | Is the goal ready? If not, ask the 1-3 questions that unblock it.         |
| `decompose`         | Turn a ready goal into 3-7 typed, HITL-annotated, DAG-ordered tasks.      |
| `decompose.critic`  | Reviews a decomposition for missing edges, unsafe tasks, weak AC.         |
| `router`            | Pick the right agent for a task when `decompose` left it open.            |
| `research`          | Retrieval-grounded question answering (see also `kb.qa`).                 |
| `browse`            | Headless-browser-assisted navigation spec.                                |
| `measure`           | Score a task outcome against its acceptance criteria.                     |
| `measure.critic`    | Second-opinion on a measure score.                                        |
| `learn`             | Distill one insight from a task outcome.                                  |
| `learn.dedup`       | Merge near-duplicate learnings before they flood the knowledge base.      |
| `memory.compact`    | Compress a long-running agent memory into a digest.                       |
| `persona.onboarding`| First-run interview — produces the user persona envelope.                 |

## Sales & CRM

| Prompt                    | Job                                                               |
| ------------------------- | ----------------------------------------------------------------- |
| `sales.qualify`           | BANT / MEDDIC classification of an inbound or sourced lead.       |
| `sales.account_research`  | Pre-meeting account brief; citation-grounded on snippets.         |
| `outreach.cold`           | Personalised cold email with CTA + compliance checks.             |
| `outreach.followup`       | Scheduled follow-up after N days of silence.                      |
| `outreach.nudge`          | Gentle commitment reminder.                                       |

## Marketing

| Prompt                      | Job                                                             |
| --------------------------- | --------------------------------------------------------------- |
| `marketing.seo_brief`       | Writer-ready SEO brief from a target keyword + SERP snapshot.   |
| `marketing.ad_variants`     | A/B-ready paid-ad copy matrix with guardrails.                  |
| `marketing.social_thread`   | Turn a source into a platform-shaped thread with citations.     |
| `exec.write`                | Long-form writing step once a brief is approved.                |
| `exec.rewrite`              | Tone / length rewrite of existing copy.                         |

## Product management

| Prompt                      | Job                                                             |
| --------------------------- | --------------------------------------------------------------- |
| `product.user_story`        | Raw feature ask → user story with Given/When/Then AC.           |
| `product.prioritize_rice`   | RICE scoring of a backlog, with a cut line.                     |
| `product.release_notes`     | Merged PRs → user-facing release notes; hides embargoed items.  |
| `exec.plan`                 | Multi-step execution plan for a single large task.              |
| `triage.ticket`             | Inbound ticket → category + severity + next step.               |

## Engineering & code lifecycle

| Prompt                      | Job                                                             |
| --------------------------- | --------------------------------------------------------------- |
| `exec.code`                 | Author a new code change given a spec.                          |
| `exec.code_review`          | Structured review of a diff with severity + actionable comments.|
| `exec.refactor`             | Refactor plan with preserved behaviour.                         |
| `code.debug`                | Triage a bug report against repo context into a minimal repro.  |
| `code.test_gen`             | Test plan for a function, language-agnostic specs.              |
| `code.pr_description`       | Staged diff → full PR body + labels + reviewers.                |
| `code.commit_msg`           | Diff summary → Conventional Commits message.                    |
| `exec.sql`                  | Natural-language → safe read-only SQL against a known schema.   |

## Ops, SRE, incident management

| Prompt                      | Job                                                             |
| --------------------------- | --------------------------------------------------------------- |
| `ops.incident_postmortem`   | Raw timeline → blameless postmortem with SMART action items.    |
| `ops.runbook_draft`         | Symptom → conservative runbook (precheck → diagnose → mitigate).|
| `ops.log_triage`            | Log window → clustered patterns + next action (no false pages). |
| `data.anomaly_detect`       | Short time-series → flagged anomalies + recommended action.     |
| `plugin.error_repair`       | Downstream API error → bounded retry / repair plan.             |

## Finance

| Prompt                 | Job                                                                  |
| ---------------------- | -------------------------------------------------------------------- |
| `finance.risk`         | Rate a proposed spend / payment for risk (always-HITL on movement).  |
| `finance.forecast`     | Short-horizon cash / spend forecast with assumptions.                |
| `finance.categorize`   | Classify transactions into a canonical chart of accounts.            |
| `finance.receipt`      | Extract line items + totals from a receipt image OCR'd to text.      |

## HR & recruiting

| Prompt                       | Job                                                            |
| ---------------------------- | -------------------------------------------------------------- |
| `hr.resume_screen`           | JD + résumé → structured fit; no protected-attribute use.      |
| `hr.interview_questions`     | Jurisdiction-aware, seniority-calibrated interview loop.       |

## Legal & compliance

| Prompt                     | Job                                                              |
| -------------------------- | ---------------------------------------------------------------- |
| `legal.contract_review`    | Clause-level risk triage with redlines; always non-advice.       |
| `data.redact`              | PII / secret redaction pass before persistence or outbound send. |
| `data.moderate`            | Policy-violation classification for free text.                   |
| `safety.injection_detect`  | Prompt-injection classifier with recommended dispatch action.    |

## Integrations (the n8n-shape primitives)

| Prompt                             | Job                                                      |
| ---------------------------------- | -------------------------------------------------------- |
| `integration.field_map`            | Source schema → target schema mapping spec.              |
| `integration.webhook_transform`    | Incoming webhook payload → internal event via JSONPath.  |
| `plugin.webhook`                   | Design a receiving webhook for a given partner.          |
| `plugin.oauth_choose`              | Choose an OAuth scope set for a new integration.         |
| `data.json_repair`                 | Force-repair malformed JSON into the caller's schema.    |

## Meetings & collaboration

| Prompt                | Job                                                                    |
| --------------------- | ---------------------------------------------------------------------- |
| `meeting.agenda`      | Inputs + goals → time-boxed agenda with owner per item.                |
| `meeting.notes`       | Transcript → structured notes: decisions, actions, parking lot.        |
| `meeting.retro`       | Retro transcript → themes + action items (keep / stop / start).        |
| `email.thread_summary`| Long thread → tl;dr + asks + commitments in under 280 chars.           |
| `outreach.escalation` | Draft an escalation message with tone calibrated to severity.          |

## Knowledge / docs

| Prompt              | Job                                                                      |
| ------------------- | ------------------------------------------------------------------------ |
| `kb.qa`             | Retrieval-grounded Q&A; refuses to answer from model memory.             |
| `doc.qa`            | Q&A against a single document.                                           |
| `doc.outline`       | Raw material → hierarchical outline.                                     |
| `exec.summarize`    | Generic summarization step.                                              |
| `exec.extract`      | Extract structured fields from free text by a schema.                    |
| `exec.classify`     | Classify free text into a caller-provided taxonomy.                      |
| `data.entity_extract`| Extract named entities with literal spans + per-type canonicalization.  |

## Personal productivity

| Prompt                         | Job                                                           |
| ------------------------------ | ------------------------------------------------------------- |
| `planner.weekly`               | One-week time-blocked plan from open goals + persona.         |
| `report.weekly`                | Backward-looking weekly summary of what happened.             |
| `task.prioritize_eisenhower`   | Tasks → urgent/important quadrants + today's top three.       |
| `outreach.notify`              | Draft a user-facing notification for a completed action.      |
| `outreach.apology`             | Calibrated apology after a commitment miss or incident.       |
| `outreach.reply`               | Reply draft inheriting prior-thread tone.                     |

## Translation & linguistics

| Prompt                | Job                                                         |
| --------------------- | ----------------------------------------------------------- |
| `exec.translate`      | Translate with glossary + tone preservation.                |
| `exec.sentiment`      | Multi-axis sentiment + confidence.                          |

## Conventions

Every prompt under `prompts/*.md`:

1. Defines its JSON output schema explicitly in a fenced block.
2. Has at least one worked example and one anti-example.
3. Has a `Refusal` section naming the structured empty response.
4. Has an `Injection hardening` clause declaring that untrusted
   input is data, not instructions.
5. Has a `Tool manifest` table mapping each output field to the
   concrete downstream action it drives.

Adding a prompt:

1. Write `prompts/<namespace>.<name>.md` using an existing prompt
   in the same namespace as a template.
2. Run `make prompts` to regenerate `core/pd/*.c` and `core/pd.h`.
3. Register it in `core/prompts.c`'s `TABLE[]`.
4. Optionally add golden / red-team fixtures under `evals/`.
5. Re-run `bash evals/run.sh` and `make`.

CI enforces the 166-LOC cap on every `.c` / `.h`, so a prompt that
blows up into a large generated file is a signal that the prompt
itself is doing too much — split it.
