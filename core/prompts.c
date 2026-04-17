#include <string.h>
#include "core/prompts.h"
#include "core/pd.h"

/*
 * Prompt registry. The embedded bodies live in core/pd and are
 * declared by the auto-generated core/pd.h. Keeping the mapping table
 * hand-maintained (rather than auto-generated) lets us rename a .md
 * file without regenerating a header, and gives one obvious place to
 * see every live prompt name.
 */
struct entry { const char *name; const char *body; };

static const struct entry TABLE[] = {
    { "system/persona",    P_SYSTEM_PERSONA    },
    { "system/guardrails", P_SYSTEM_GUARDRAILS },
    { "clarify",           P_CLARIFY           },
    { "decompose",         P_DECOMPOSE         },
    { "router",            P_ROUTER            },
    { "research",          P_RESEARCH          },
    { "browse",            P_BROWSE            },
    { "measure",           P_MEASURE           },
    { "learn",             P_LEARN             },
    { "outreach.nudge",    P_OUTREACH_NUDGE    },
    { "outreach.notify",   P_OUTREACH_NOTIFY   },
    { "finance.risk",      P_FINANCE_RISK      },
    { "plugin.webhook",    P_PLUGIN_WEBHOOK    },
};
#define N_PROMPTS (sizeof(TABLE) / sizeof(TABLE[0]))

static const char *NAMES[N_PROMPTS + 1];
static int names_built = 0;

const char *prompt_get(const char *name)
{
    size_t i;
    if (!name) return 0;
    for (i = 0; i < N_PROMPTS; i++)
        if (strcmp(TABLE[i].name, name) == 0) return TABLE[i].body;
    return 0;
}

const char *const *prompt_list(void)
{
    size_t i;
    if (!names_built) {
        for (i = 0; i < N_PROMPTS; i++) NAMES[i] = TABLE[i].name;
        NAMES[N_PROMPTS] = 0;
        names_built = 1;
    }
    return NAMES;
}

size_t prompt_count(void) { return N_PROMPTS; }
