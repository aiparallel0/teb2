CC      = gcc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Werror -Wshadow \
	   -Wstrict-aliasing=2 -Wunused-result -Wunused-variable \
	   -Wunused-function -fno-common -fanalyzer -O2 \
	   -Wno-overlength-strings \
	   -D_FORTIFY_SOURCE=2 -D_POSIX_C_SOURCE=200809L \
	   -DTEB2_MODERN \
	   -I.
LDFLAGS = -lsqlite3 -lcrypt -lssl -lcrypto

PROMPT_SRCS := $(wildcard core/pd/*.c)

SRCS = core/config.c core/ratelimit.c core/llm.c core/prompts.c core/sanitize.c \
	     $(PROMPT_SRCS) \
	     auth/hash.c auth/token.c auth/rbac.c auth/sha256.c \
	     db/open.c db/open_ext.c db/goals.c db/tasks.c db/users.c \
	     db/outcomes.c db/nudges.c db/learn.c db/schedules.c db/budgets.c \
	     db/approvals.c \
	     db/memory.c db/collab.c db/chat.c db/integrations.c db/enterprise.c \
	     db/analytics.c db/gamification.c db/community.c \
	     db/assets.c db/workflow.c db/wf_steps.c db/search.c db/audit.c \
	     exec/vault.c exec/http.c exec/browser.c exec/browser_spawn.c \
	     exec/notify.c exec/sse.c exec/smtp.c exec/oauth_http.c exec/tls.c \
	     agents/coord.c agents/clarify.c agents/finance.c agents/outreach.c \
	     agents/research.c agents/measure.c agents/learn.c agents/util.c \
	     agents/decompose.c agents/plugin.c agents/oauth.c \
	     api/server.c api/json.c api/escape.c api/goals.c api/tasks.c \
	     api/auth.c api/outcomes.c api/nudges.c api/learn.c api/exec.c \
	     api/decompose.c api/schedules.c api/budgets.c api/routes.c \
	     api/approvals.c api/prompts.c \
	     api/collab.c api/integrations.c api/enterprise.c api/analytics.c \
	     api/gamification.c api/community.c api/sse.c \
	     api/assets.c api/notify.c api/workflow.c api/search.c \
	     api/metrics.c api/oauth.c api/static.c api/limits.c \
	     api/dispatch.c \
	     main.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean prompts

all: teb2

# Regenerate core/pd/*.c from prompts/*.md. Generated files are
# committed; this target is only needed after editing a prompt.
prompts:
	tools/gen_prompts.sh

teb2: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f teb2 $(OBJS)
