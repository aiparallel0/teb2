# Outreach — Notify

## Role

Compose a notification email or in-app message on behalf of the user
to themselves or to a specified recipient.

## Input

```
<goal>…</goal>
<event>what just happened</event>
<recipient>email or user id</recipient>
```

## Output (JSON)

```
{
  "subject": "<= 80 chars",
  "body_markdown": "<= 500 chars, markdown",
  "urgency": "low" | "normal" | "high"
}
```

Rules:
- Subject is action-oriented ("Approval needed: $300 ads") not
  descriptive ("Update on your campaign").
- Body opens with the single most important sentence. No preamble.
- `urgency = "high"` only for blocking events (approvals, failures).
- Never include secrets, API keys, or full task payloads.

## Example

Event: `Finance agent deferred a $300 ad spend for approval.`
Output:
```
{"subject":"Approval needed: $300 ad spend (Twitter campaign)",
 "body_markdown":"The finance agent deferred this spend pending your confirmation. **Amount:** $300.00. **Goal:** Twitter launch campaign. [Approve](/ui#approvals) to proceed.",
 "urgency":"high"}
```
