#!/usr/bin/env python3
"""
Mock LLM server for teb2 eval harness.

Listens on http://127.0.0.1:$PORT/v1/chat/completions and replies
with a canned JSON response based on a simple heuristic over the
incoming system + user messages. This lets the eval harness exercise
every agent without spending OpenAI credits or depending on network.

Fixtures are defined inline and keyed on the prompt name embedded in
the system message by core/prompts.{h,c}. If a request does not match
any fixture, a 503 is returned so the test surfaces the miss rather
than silently masking it.

Limitations: this is a smoke-test mock, not a semantic simulator.
Follow-ups (docs/FOLLOWUPS.md §J) add prompt-quality eval with real
LLM calls scored against a rubric.
"""
import json
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

FIXTURES = {
    # prompt_name fragment -> canned assistant reply (as a string)
    "clarify":        '{"status":"ready","questions":[],"readiness_score":85,"missing_dimensions":[],"reasoning":"goal appears specific enough"}',
    "decompose":      '{"tasks":[{"title":"Research market","description":"find comparable products","agent":"research","depends_on":[],"effort_minutes":45,"est_cost_cents":0,"requires_hitl":false,"success_criteria":"3 comparables identified"},{"title":"Draft outreach","description":"write intro email","agent":"outreach","depends_on":[0],"effort_minutes":20,"est_cost_cents":0,"requires_hitl":false,"success_criteria":"email saved"}]}',
    "measure":        '{"score_0_100":72,"rubric":[{"criterion":"outcome present","met":true,"evidence":"result text provided"}],"next_action":"retry","reasoning":"partial completion"}',
    "learn":          '{"insight":"short focused drafts outperform long ones","tags":["writing","outreach"],"generalizes_to":"drafting any short-form communication","confidence":0.7,"evidence_excerpt":"outcome indicates reply rate doubled with brevity"}',
    "research":       '{"summary":"overview unavailable without grounded sources","key_facts":[],"confidence":"low","open_questions":["needs web access"]}',
    "browse":         '{"plan":[{"action":"click","selector":"button#go","value":""}],"stop_conditions":["URL matches /done"],"requires_confirmation":false,"explanation":"mock plan"}',
    "router":         '{"agent":"exec","confidence":0.6,"rationale":"default bucket"}',
    "finance.risk":       '{"risk":"LOW","signals":["amount_small","known_user"],"recommend":"auto","reasoning":"low absolute value"}',
    "finance.forecast":   '{"projected_cents":0,"projected_by_category":{},"method":"insufficient_data","confidence":"low","overrun":{"overall":false,"by_category":[]},"top_drivers":[],"warnings":["mock"]}',
    "finance.categorize": '{"category":"saas","confidence":0.9,"signals":["vendor_match"],"alternate":null,"reasoning":"mock"}',
    "finance.receipt":    '{"vendor":"Mock","purchased_at":"2026-01-01","currency":"USD","line_items":[],"subtotal_cents":0,"tax_cents":0,"tip_cents":0,"total_cents":0,"payment_method":null,"integrity":{"sum_matches":true,"rounding_issue":false}}',
    "outreach.nudge":     '{"message":"Lock the brief before you touch the copy.","tone":"challenging","references_learnings":[]}',
    "outreach.notify":    '{"subject":"Approval needed","body_markdown":"Please approve.","urgency":"normal"}',
    "outreach.reply":     '{"subject":"Re: mock","body_markdown":"mock reply","addresses_questions":[],"unanswered_questions":[],"follow_up_needed":false,"suggested_send_delay_minutes":30}',
    "outreach.cold":      '{"subject":"mock subject","body_markdown":"mock body","opener":"mock opener","cta_sentence":"mock cta","compliance":{"has_identifying_sender":true,"has_unsubscribe_line":true,"no_false_claims":true}}',
    "outreach.followup":  '{"action":"wait","subject":"","body_markdown":"","rationale":"too soon","next_check_in_days":3}',
    "outreach.apology":   '{"subject":"Mock outage","body_markdown":"mock","banned_phrases_detected":[],"accountability_score":80}',
    "plugin.webhook":       '{"headers":{"Content-Type":"application/json"},"body_json":{"text":"mock"}}',
    "plugin.oauth_choose":  '{"chosen_scopes":["repo:status"],"rejected_scopes":["repo"],"risk":"low","rationale":"least privilege","over_request_detected":true}',
    "plugin.error_repair":  '{"can_repair":false,"repair_kind":"none","patched_body_json":null,"changes":[],"retry_advice":{"wait_seconds":0,"max_attempts":0},"explanation":"mock","risk_notes":[]}',
    "exec.code":          '{"filename":"mock.py","code":"print(1)\\n","language":"python","entry_point":"python mock.py","assumptions":[],"not_covered":[]}',
    "exec.code_review":   '{"verdict":"approve","summary":"mock","findings":[],"missing_tests":[],"positive_notes":["mock"]}',
    "exec.refactor":      '{"summary":"mock","steps":[{"index":0,"description":"mock","affected_files":["x"],"risk":"low","reversible":true,"validated_by":"build"}],"invariants":[],"rollback_plan":"revert","out_of_scope":[]}',
    "exec.write":         '{"title":"Mock","subtitle":"","body_markdown":"mock body","call_to_action":"do it","word_count":2,"includes_verification":{"must_include_hits":[],"must_avoid_hits":[]}}',
    "exec.summarize":     '{"summary":"mock","bullets":[],"key_entities":[],"unsupported_claims_detected":[],"truncated":false}',
    "exec.extract":       '{"data":{},"missing":[],"confidence":{},"evidence":{}}',
    "exec.classify":      '{"labels":["other"],"scores":{"other":0.6},"primary":"other","below_threshold":false,"rationale":"mock"}',
    "exec.sql":           '{"sql":"SELECT 1;","kind":"select","read_only_violation":false,"estimated_rows":"low","safety_notes":[],"parameters":[]}',
    "exec.translate":     '{"translation":"mock","detected_source_lang":"en","glossary_applied":[],"preserved_spans":[],"notes":[]}',
    "exec.rewrite":       '{"rewritten":"mock","changed_claims":[],"length_ratio":1.0,"warnings":[]}',
    "exec.sentiment":     '{"polarity":"neutral","score":0.0,"intensity":"low","emotions":[],"intent":"other","aspect_scores":{},"evidence":[],"is_sarcastic":false}',
    "exec.plan":          '{"approach":"mock","steps":[{"index":0,"action":"mock","output_artifact":"x","verify_by":"build"}],"risks":[],"open_questions":[],"est_effort_minutes":60}',
    "data.redact":        '{"redacted_text":"mock","replacements":[],"coverage":{},"residual_risk":"low","residual_notes":[]}',
    "data.moderate":      '{"action":"allow","labels":[],"applied_rule":"default","requires_human":false,"explanation":"mock"}',
    "data.json_repair":   '{"repaired":{},"diagnostics":[],"schema_compliant":true,"dropped_keys":[],"unchanged":true}',
    "meeting.agenda":     '{"title":"Mock","agenda":[],"total_minutes":0,"open_decisions_covered":[],"parking_lot":[]}',
    "meeting.notes":      '{"title":"Mock","summary":"mock","decisions":[],"action_items":[],"open_questions":[],"attendance":[],"risks":[]}',
    "meeting.retro":      '{"summary":"mock","start":[],"stop":[],"continue":[],"experiments":[],"themes":[],"kudos":[]}',
    "doc.qa":             '{"answer":"mock","citations":[],"confidence":"low","cannot_answer_reason":"missing","suggested_followups":[]}',
    "doc.outline":        '{"title":"Mock","tldr":"mock","sections":[],"cta":"none","uncovered":[]}',
    "triage.ticket":      '{"category":"other","secondary_categories":[],"priority":"p3","queue":"t1_support","sla_due_utc":"2026-01-01T00:00:00Z","signals":[],"sentiment":"calm","escalate_to_human":false,"duplicate_of":null,"suggested_macro":null}',
}

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):  # noqa: N802
        n = int(self.headers.get("content-length", "0"))
        body = self.rfile.read(n).decode("utf-8", errors="replace")
        reply = self._match(body)
        if reply is None:
            self.send_response(503)
            self.send_header("content-type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error":"no_fixture"}')
            return
        env = {
            "choices": [{"message": {"role": "assistant", "content": reply}}],
            "usage":   {"total_tokens": 123},
        }
        payload = json.dumps(env).encode("utf-8")
        self.send_response(200)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def _match(self, body):
        for name, reply in FIXTURES.items():
            if name in body:
                return reply
        return None

    def log_message(self, fmt, *args):  # silence default access log
        return

def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8088
    srv = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    sys.stderr.write("mock_llm listening on %d\n" % port)
    srv.serve_forever()

if __name__ == "__main__":
    main()
