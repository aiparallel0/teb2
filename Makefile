CC      = gcc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Werror -Wshadow \
           -Wstrict-aliasing=2 -Wunused-result -Wunused-variable \
           -Wunused-function -fno-common -fanalyzer -O2 \
           -D_FORTIFY_SOURCE=2 -D_POSIX_C_SOURCE=200809L \
           -DTEB2_MODERN \
           -I.
LDFLAGS = -lsqlite3 -lcrypt

SRCS = core/config.c \
       auth/hash.c auth/token.c auth/rbac.c auth/sha256.c \
       db/open.c db/open_ext.c db/goals.c db/tasks.c db/users.c \
       db/outcomes.c db/nudges.c db/learn.c db/schedules.c db/budgets.c \
       db/memory.c db/collab.c db/chat.c db/integrations.c db/enterprise.c \
       db/analytics.c db/gamification.c db/community.c \
       exec/vault.c exec/http.c exec/browser.c exec/notify.c \
       agents/coord.c agents/clarify.c agents/finance.c agents/outreach.c \
       agents/research.c agents/measure.c agents/learn.c agents/util.c \
       agents/decompose.c agents/plugin.c agents/oauth.c \
       api/server.c api/json.c api/escape.c api/goals.c api/tasks.c \
       api/auth.c api/outcomes.c api/nudges.c api/learn.c api/exec.c \
       api/decompose.c api/schedules.c api/budgets.c api/routes.c \
       api/collab.c api/integrations.c api/enterprise.c api/analytics.c \
       api/gamification.c api/community.c \
       main.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: teb2

teb2: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f teb2 $(OBJS)
