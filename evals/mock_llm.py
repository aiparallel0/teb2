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
    "measure":        '{"score_0_100":72,"rubric":[{"criterion":"outcome present","met":true,"evidence":"result text provided"}],"next_action":"advance","reasoning":"partial completion"}',
    "learn":          '{"insight":"short focused drafts outperform long ones","tags":["writing","outreach"],"generalizes_to":"drafting any short-form communication","confidence":0.7,"evidence_excerpt":"outcome indicates reply rate doubled with brevity"}',
    "research":       '{"summary":"overview unavailable without grounded sources","key_facts":[],"confidence":"low","open_questions":["needs web access"]}',
    "finance.risk":   '{"risk":"LOW","signals":["amount_small","known_user"],"recommend":"auto","reasoning":"low absolute value"}',
    "outreach.nudge": '{"message":"You are close — one small push today keeps the momentum.","tone":"encouraging","references_learnings":[]}',
    "router":         '{"agent":"exec","confidence":0.6,"rationale":"default bucket"}',
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
